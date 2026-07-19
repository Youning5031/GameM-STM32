#pragma once

#include "extime.h"
#include "main.h"
#include "spi.h"
#include "StaticQueue.hpp"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_dma.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_spi.h"

#include <cstddef>
#include <cstdint>
#include <span>

extern DMA_HandleTypeDef hdma_spi1_tx;

struct IrqGuard {
    IrqGuard() {
        __disable_irq();
    }
    ~IrqGuard() {
        __enable_irq();
    }
    // 禁止拷贝和移动
    IrqGuard(const IrqGuard &) = delete;
    IrqGuard &operator=(const IrqGuard &) = delete;
    void disable() {
        __disable_irq();
    };
    void enable() {
        __enable_irq();
    };
};

struct TransferRequest {
private:
    union {
        std::uint8_t raw[8];  // 原始字节，便于整体拷贝
        struct {
            std::uint8_t data[7];     // 内嵌数据（最多 7 字节）
            std::uint8_t data_length;  // 有效长度（0~7）
        };  // isPointer == false 时使用
        struct {
            std::uint8_t *ptr;      // 存储指针值（32位平台）
            std::size_t ptr_length;  // 缓冲区长度
        };  // isPointer == true 时使用
    };
    bool is_pointer;

public:
    bool is_command = false, is_transmit = true;

    template<std::convertible_to<std::uint8_t>... Args>
    TransferRequest(Args... data) : data{data...}, data_length(sizeof...(data)), is_pointer(false) {
        static_assert(sizeof...(data) <= 7, "参数数量过多，应使用指针");
    }
    template<typename T, std::size_t Extent>
        requires(!std::is_const_v<T>)
    TransferRequest(std::span<T, Extent> data)
      : ptr(reinterpret_cast<uint8_t *>(data.data())), ptr_length(data.size_bytes()), is_transmit(false) {}
    template<typename T, std::size_t Extent>
        requires std::is_const_v<T>
    TransferRequest(std::span<T, Extent> data)
      : ptr(reinterpret_cast<uint8_t *>(const_cast<std::remove_const_t<T> *>(data.data())))
      , ptr_length(data.size_bytes())
      , is_transmit(true) {}

    TransferRequest &setCommandMode() {
        is_command = true;
        return *this;
    }
    TransferRequest &setDataMode() {
        is_command = false;
        return *this;
    }
    TransferRequest &setTransmitMode() {
        is_transmit = true;
        return *this;
    }
    TransferRequest &setReceiveMode() {
        is_transmit = false;
        return *this;
    }

    std::pair<std::uint8_t *, std::size_t> getData() {
        if (is_pointer) {
            return {ptr, ptr_length};
        } else {
            return {data, static_cast<std::size_t>(data_length)};
        }
    }

    std::pair<std::uint8_t *, std::size_t> operator*() {
        return getData();
    }
};

inline int busyc{0};
inline int intc{0};

class SpiDmaEngine {
    friend void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi);
    friend void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi);
    friend void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi);
    friend void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi);

public:
    static constexpr std::size_t MAX_QUEUE = 32;
    static constexpr int DEFAULT_RETRY = 8;

    SpiDmaEngine(const SpiDmaEngine &) = delete;
    SpiDmaEngine &operator=(const SpiDmaEngine &) = delete;
    SpiDmaEngine(SpiDmaEngine &&) = delete;
    SpiDmaEngine &operator=(SpiDmaEngine &&) = delete;

private:
    StaticQueue<TransferRequest, MAX_QUEUE> queue;
    volatile bool is_busy = false;
    // SpiDmaEngine(DMA_HandleTypeDef &hdma){};
    SpiDmaEngine() {}
    // SpiDmaEngine() {
    //     HAL_DMA_RegisterCallback(&hdma_spi1_tx, HAL_DMA_XFER_CPLT_CB_ID, [](DMA_HandleTypeDef *) {
    //         SpiDmaEngine::getInstance().callback();
    //     });
    // }
    // ~SpiDmaEngine() {
    //     HAL_DMA_UnRegisterCallback(&hdma_spi1_tx, HAL_DMA_XFER_CPLT_CB_ID);
    // }

    bool startTrans(TransferRequest &req) {
        is_busy = true;
        busyc++;
        HAL_GPIO_WritePin(SCREEN_CS_GPIO_Port, SCREEN_CS_Pin, GPIO_PIN_RESET);
        if (req.is_command) HAL_GPIO_WritePin(SCREEN_DC_GPIO_Port, SCREEN_DC_Pin, GPIO_PIN_RESET);
        else HAL_GPIO_WritePin(SCREEN_DC_GPIO_Port, SCREEN_DC_Pin, GPIO_PIN_SET);
        auto [ptr, len] = *req;
        HAL_StatusTypeDef status;
        for(auto _ = 0; _<100;_++)__NOP();
        if (req.is_transmit) {
            status = HAL_SPI_Transmit_DMA(&hspi1, ptr, len);
        } else {
            status = HAL_SPI_Receive_DMA(&hspi1, ptr, len);
        }
        if (status != HAL_OK) {
            HAL_GPIO_WritePin(SCREEN_CS_GPIO_Port, SCREEN_CS_Pin, GPIO_PIN_SET);
            is_busy = false;
            return false;
        }
        return true;
    }

    void endTrans() {
        HAL_GPIO_WritePin(SCREEN_CS_GPIO_Port, SCREEN_CS_Pin, GPIO_PIN_SET);
        queue.pop();
        is_busy = false;
        busyc--;
    }

    void callback() {
        intc--;
        endTrans();
        if (queue.isEmpty()) return;
        int timesout = DEFAULT_RETRY / 2;
        while (!startTrans(*queue.peek())) {
            if (timesout == 0) Error_Handler();
            timesout--;
        }
    }

    void errorCallback() {
        HAL_GPIO_WritePin(SCREEN_CS_GPIO_Port, SCREEN_CS_Pin, GPIO_PIN_SET);
        is_busy = false;
        if (queue.isEmpty()) return;
        if (!startTrans(*queue.peek())) {
            Error_Handler();
        }
    }

public:
    static SpiDmaEngine &getInstance() {
        static SpiDmaEngine instance;
        return instance;
    }

    bool submit(const TransferRequest &req) {
        IrqGuard guard{};
        intc++;
        while (!queue.push(req)) {
            guard.enable();
            __WFI();
            guard.disable();
        }

        if (!is_busy && hdma_spi1_tx.State == HAL_DMA_STATE_READY) {
            int timesout = DEFAULT_RETRY;
            while (!startTrans(*queue.peek())) {
                if (timesout == 0) return false;
                timesout--;
            }
        }
        return true;
    }

    void wait() {
        IrqGuard guard{};
        if (!is_busy && hdma_spi1_tx.State == HAL_DMA_STATE_READY) {
            int timesout = DEFAULT_RETRY * 2;
            while (!startTrans(*queue.peek())) {
                if (timesout == 0) Error_Handler();
                timesout--;
            }
        }

        while (is_busy) {
            guard.enable();
            __WFI();
            guard.disable();
        }
    }
};
