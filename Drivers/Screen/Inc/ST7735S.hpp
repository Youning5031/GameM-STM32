#pragma once

#include "Screen.hpp"
#include "extime.h"
#include "literalunit.hpp"
#include "main.h"
#include "Point.hpp"
#include "SpiDmaEngine.hpp"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_spi.h"

#include <algorithm>
#include <assert.h>
#include <concepts>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>

/*
 * _DT: DisplayTraits 结构体模板实例，
 * frame_buffer: _DT::BufferType 类型的缓冲区
 */
template<typename _DT, auto &buffer>  // 有缓冲区版本
class ST7735S {
    friend class Screen<ST7735S>;
public:
    using Color = typename _DT::Color;
    constexpr static inline auto WIDTH = _DT::WIDTH;
    constexpr static inline auto HEIGHT = _DT::HEIGHT;
    static constexpr auto PIXEL_COUNT = _DT::PIXEL_COUNT;
    using BufferType = typename _DT::BufferType;
private:
    constexpr static inline auto&& framebuffer = buffer;

public:
    enum class Command : std::uint8_t {
        NOP = 0x00U,        // No Operation
        SWRESET = 0x01U,    // Software reset
        RDDID = 0x04U,      // Read Display ID
        RDDST = 0x09U,      // Read Display Statu
        RDDPM = 0x0AU,      // Read Display Power
        RDDMADCTL = 0x0BU,  // Read Display
        RDDCOLMOD = 0x0CU,  // Read Display Pixel
        RDDIM = 0x0DU,      // Read Display Image
        RDDSM = 0x0EU,      // Read Display Signal
        SLPIN = 0x10U,      // Sleep in & booster off
        SLPOUT = 0x11U,     // Sleep out & booster on
        PTLON = 0x12U,      // Partial mode on
        NORON = 0x13U,      // Partial off (Normal)
        INVOFF = 0x20U,     // Display inversion off
        INVON = 0x21U,      // Display inversion on
        GAMSET = 0x26U,     // Gamma curve select
        DISPOFF = 0x28U,    // Display off
        DISPON = 0x29U,     // Display on
        CASET = 0x2AU,      // Column address set
        RASET = 0x2BU,      // Row address set
        RAMWR = 0x2CU,      // Memory write
        RGBSET = 0x2DU,     // LUT for 4k,65k,262k color
        RAMRD = 0x2EU,      // Memory read
        PTLAR = 0x30U,      // Partial start/end address set
        TEOFF = 0x34U,      // Tearing effect line off
        TEON = 0x35U,       // Tearing effect mode set & on
        MADCTL = 0x36U,     // Memory data access control
        IDMOFF = 0x38U,     // Idle mode off
        IDMON = 0x39U,      // Idle mode on
        COLMOD = 0x3AU,     // Interface pixel format
        FRMCTR1 = 0xB1U,    // In normal mode (Full colors)
        FRMCTR2 = 0xB2U,    // In Idle mode (8-colors)
        FRMCTR3 = 0xB3U,    // In partial mode + Full colors
        INVCTR = 0xB4U,     // Display inversion control
        // DISPLAY_SETTING = 0xB6U,  // Display function setting // 注释说明：手册中不存在此命令，下同
        PWCTR1 = 0xC0U,    // Power control setting
        PWCTR2 = 0xC1U,    // Power control setting
        PWCTR3 = 0xC2U,    // In normal mode (Full colors)
        PWCTR4 = 0xC3U,    // In Idle mode (8-colors)
        PWCTR5 = 0xC4U,    // In partial mode + Full colors
        VMCTR1 = 0xC5U,    // VCOM control 1
        VMOFCTR = 0xC7U,   // Set VCOM offset control
        WRID2 = 0xD1U,     // Set LCM version code
        WRID3 = 0xD2U,     // Customer Project code
        NVCTR1 = 0xD9U,    // NVM control status
        RDID1 = 0xDAU,     // Read ID1
        RDID2 = 0xDBU,     // Read ID2
        RDID3 = 0xDCU,     // Read ID3
        NVCTR2 = 0xDEU,    // NVM Read Command
        NVCTR3 = 0xDFU,    // NVM Write Command
        GAMCTRP1 = 0xE0U,  // Set Gamma adjustment (+ polarity)
        GAMCTRN1 = 0xE1U,  // Set Gamma adjustment (- polarity)
        // EXT_CTRL = 0xF0U,         // Extension command control
        PWCTR6 = 0xFCU,  // In partial mode + Idle mode
        // VCOM4_LEVEL = 0xFFU,      // VCOM 4 level control
    };

private:
    std::optional<std::pair<Point, Point>> clipRect(Point pos, Point diagonal) {
        // 确保在 pos 和 end 的位置关系为左上、右下
        Point end = pos + diagonal;
        if (end.x < pos.x) std::swap(end.x, pos.x);
        if (end.y < pos.y) std::swap(end.y, pos.y);

        // 确保矩形屏幕范围内
        if (end.x < 0 || end.y < 0 || pos.x >= static_cast<Point::pType>(WIDTH)
            || pos.y >= static_cast<Point::pType>(HEIGHT))
            return std::nullopt;
        pos.x = std::clamp(pos.x, static_cast<Point::pType>(0), static_cast<Point::pType>(WIDTH - 1));
        pos.y = std::clamp(pos.y, static_cast<Point::pType>(0), static_cast<Point::pType>(HEIGHT - 1));
        end.x = std::clamp(end.x, static_cast<Point::pType>(0), static_cast<Point::pType>(WIDTH - 1));
        end.y = std::clamp(end.y, static_cast<Point::pType>(0), static_cast<Point::pType>(HEIGHT - 1));

        return std::pair{pos, end};
    };

    void updateBuffer(Point pos, Point diagonal){
        // 规范化矩形
        auto result = clipRect(pos, diagonal);
        if (!result) return;
        auto [start, end] = *result;
        diagonal = end - start;
        // 计算绘制区域大小
        auto size = static_cast<std::size_t>((diagonal.x + 1) * (diagonal.y + 1));
        setWindow(start, end);
        sendCommand(Command::RAMWR, std::span<Color>{framebuffer,size});
    }

public:
    void sendData(std::uint8_t data) const {
        // SpiDmaEngine::getInstance().submit(TransferRequest(data).setDataMode().setTransmitMode());
        HAL_GPIO_WritePin(SCREEN_CS_GPIO_Port, SCREEN_CS_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(SCREEN_DC_GPIO_Port, SCREEN_DC_Pin, GPIO_PIN_SET);
        HAL_SPI_Transmit(&hspi1, &data, 1, HAL_TIMEOUT);
        HAL_GPIO_WritePin(SCREEN_CS_GPIO_Port, SCREEN_CS_Pin, GPIO_PIN_SET);
    }

    template<typename T, std::size_t Extent>
    void sendData(std::span<T, Extent> data) const {
        if (data.empty()) return;
        // SpiDmaEngine::getInstance().submit(TransferRequest(data).setDataMode().setTransmitMode());
        HAL_GPIO_WritePin(SCREEN_CS_GPIO_Port, SCREEN_CS_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(SCREEN_DC_GPIO_Port, SCREEN_DC_Pin, GPIO_PIN_SET);
        HAL_SPI_Transmit(&hspi1, reinterpret_cast<std::uint8_t *>(data.data()), data.size_bytes(), HAL_TIMEOUT);
        HAL_GPIO_WritePin(SCREEN_CS_GPIO_Port, SCREEN_CS_Pin, GPIO_PIN_SET);
    }
    template<std::convertible_to<std::uint8_t>... Args>
    void sendData(Args... args) const {
        if constexpr (sizeof...(args) != 0) {
            // SpiDmaEngine::getInstance().submit(TransferRequest(args...).setDataMode().setTransmitMode());
            std::array<std::uint8_t,sizeof...(args)> data{args...};
            HAL_GPIO_WritePin(SCREEN_CS_GPIO_Port, SCREEN_CS_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(SCREEN_DC_GPIO_Port, SCREEN_DC_Pin, GPIO_PIN_SET);
            HAL_SPI_Transmit(&hspi1, data.data(), data.size(), HAL_TIMEOUT);
            HAL_GPIO_WritePin(SCREEN_CS_GPIO_Port, SCREEN_CS_Pin, GPIO_PIN_SET);
        }
    }

    template<std::convertible_to<std::uint8_t>... Args>
    void sendCommand(Command cmd, Args... args) const {
        // SpiDmaEngine &e = SpiDmaEngine::getInstance();
        // e.submit(TransferRequest(static_cast<std::uint8_t>(cmd)).setCommandMode().setTransmitMode());
        auto c = static_cast<std::uint8_t>(cmd);
        HAL_GPIO_WritePin(SCREEN_CS_GPIO_Port, SCREEN_CS_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(SCREEN_DC_GPIO_Port, SCREEN_DC_Pin, GPIO_PIN_RESET);
        HAL_SPI_Transmit(&hspi1, &c, 1, HAL_TIMEOUT);
        HAL_GPIO_WritePin(SCREEN_CS_GPIO_Port, SCREEN_CS_Pin, GPIO_PIN_SET);
        if constexpr (sizeof...(args) != 0) {
            // e.submit(TransferRequest(args...).setDataMode().setTransmitMode());
            std::array<std::uint8_t,sizeof...(args)> data{args...};
            HAL_GPIO_WritePin(SCREEN_CS_GPIO_Port, SCREEN_CS_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(SCREEN_DC_GPIO_Port, SCREEN_DC_Pin, GPIO_PIN_SET);
            HAL_SPI_Transmit(&hspi1, data.data(), data.size(), HAL_TIMEOUT);
            HAL_GPIO_WritePin(SCREEN_CS_GPIO_Port, SCREEN_CS_Pin, GPIO_PIN_SET);
        }
    }
    template<typename T, std::size_t Extent>
    void sendCommand(Command cmd, std::span<T, Extent> data) const {
        // SpiDmaEngine &e = SpiDmaEngine::getInstance();
        // e.submit(TransferRequest(static_cast<std::uint8_t>(cmd)).setCommandMode().setTransmitMode());
        auto c = static_cast<std::uint8_t>(cmd);
        HAL_GPIO_WritePin(SCREEN_CS_GPIO_Port, SCREEN_CS_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(SCREEN_DC_GPIO_Port, SCREEN_DC_Pin, GPIO_PIN_RESET);
        HAL_SPI_Transmit(&hspi1, &c, 1, HAL_TIMEOUT);
        HAL_GPIO_WritePin(SCREEN_CS_GPIO_Port, SCREEN_CS_Pin, GPIO_PIN_SET);
        if (!data.empty()) {
            // e.submit(TransferRequest(data).setDataMode().setTransmitMode());
            HAL_GPIO_WritePin(SCREEN_CS_GPIO_Port, SCREEN_CS_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(SCREEN_DC_GPIO_Port, SCREEN_DC_Pin, GPIO_PIN_SET);
            HAL_SPI_Transmit(&hspi1, reinterpret_cast<std::uint8_t *>(data.data()), data.size_bytes(), HAL_TIMEOUT);
            HAL_GPIO_WritePin(SCREEN_CS_GPIO_Port, SCREEN_CS_Pin, GPIO_PIN_SET);
        }
    }

    bool init() {
        // 硬复位
        HAL_GPIO_WritePin(SCREEN_RST_GPIO_Port, SCREEN_RST_Pin, GPIO_PIN_RESET);
        delayMicros(50);
        HAL_GPIO_WritePin(SCREEN_RST_GPIO_Port, SCREEN_RST_Pin, GPIO_PIN_SET);
        delayMillis(150);

        // 软复位、唤醒
        sendCommand(Command::SWRESET);
        wait();
        delayMillis(150);
        sendCommand(Command::SLPOUT);
        wait();
        delayMillis(150);

        // 帧率控制
        sendCommand(Command::FRMCTR1, 0x01_U8, 0x2C_U8, 0x2D_U8);
        sendCommand(Command::FRMCTR2, 0x01_U8, 0x2C_U8, 0x2D_U8);
        sendCommand(Command::FRMCTR3, 0x01_U8, 0x2C_U8, 0x2D_U8, 0x01_U8, 0x2C_U8, 0x2D_U8);

        // 电源管理
        sendCommand(Command::PWCTR1, 0xA2_U8, 0x02_U8, 0x84_U8);
        sendCommand(Command::PWCTR2, 0xC5_U8);
        sendCommand(Command::PWCTR3, 0x0A_U8, 0x00_U8);
        sendCommand(Command::PWCTR4, 0x8A_U8, 0x2A_U8);
        sendCommand(Command::PWCTR5, 0x8A_U8, 0xEE_U8);
        sendCommand(Command::VMCTR1, 0x0E_U8);

        // Gamma 校正
        sendCommand(Command::GAMCTRP1, 0x02_U8, 0x1C_U8, 0x07_U8, 0x12_U8, 0x37_U8, 0x32_U8, 0x29_U8);
        sendData(0x2D_U8, 0x29_U8, 0x25_U8, 0x2B_U8, 0x39_U8, 0x00_U8, 0x01_U8);
        sendData(0x03_U8, 0x10_U8);
        sendCommand(Command::GAMCTRN1, 0x03_U8, 0x1D_U8, 0x07_U8, 0x06_U8, 0x2E_U8, 0x2C_U8, 0x29_U8);
        sendData(0x2D_U8, 0x2E_U8, 0x2E_U8, 0x37_U8, 0x3F_U8, 0x00_U8, 0x00_U8);
        sendData(0x02_U8, 0x10_U8);

        // 色彩管理
        sendCommand(Command::COLMOD, 0x05_U8);

        // 开启显示
        sendCommand(Command::INVOFF);
        sendCommand(Command::INVCTR, 0x07_U8);
        sendCommand(Command::NORON);
        sendCommand(Command::DISPON);
        wait();
        delayMillis(10);

        HAL_GPIO_WritePin(SCREEN_BL_GPIO_Port, SCREEN_BL_Pin, GPIO_PIN_SET);

        return true;
    }

    // 仿真使用ST7735R
    // bool init() {
    //     // 硬复位
    //     HAL_GPIO_WritePin(SCREEN_RST_GPIO_Port, SCREEN_RST_Pin, GPIO_PIN_RESET);
    //     delayMicros(50);
    //     HAL_GPIO_WritePin(SCREEN_RST_GPIO_Port, SCREEN_RST_Pin, GPIO_PIN_SET);
    //     delayMillis(150);

    //     sendCommand(Command::SLPOUT);  // Sleep out
    //     delayMillis(120);      // Delay 120ms
    //     //------------------------------------ST7735S Frame Rate-----------------------------------------//
    //     sendCommand(Command::FRMCTR1, 0x05_U8);

    //     sendCommand(Command::SLPOUT);  // Sleep exit
    //     delayMillis(120);
    //     // ST7735R Frame Rate
    //     sendCommand(Command::FRMCTR1, 0x01_U8, 0x2C_U8, 0x2D_U8);
    //     sendCommand(Command::FRMCTR2, 0x01_U8, 0x2C_U8, 0x2D_U8);
    //     sendCommand(Command::FRMCTR3, 0x01_U8, 0x2C_U8, 0x2D_U8, 0x01_U8, 0x2C_U8, 0x2D_U8);

    //     sendCommand(Command::INVCTR, 0x07_U8);
    //     // ST7735R Power Sequence
    //     sendCommand(Command::PWCTR1, 0xA2_U8, 0x02_U8, 0x84_U8);
    //     sendCommand(Command::PWCTR2, 0x0A_U8, 0x00_U8);
    //     sendCommand(Command::PWCTR4, 0x8A_U8, 0x2A_U8);
    //     sendCommand(Command::PWCTR5, 0x8A_U8, 0xEE_U8);
    //     sendCommand(Command::VMCTR1, 0x0E_U8);
    //     sendCommand(Command::MADCTL, 0xC8_U8);

    //     // ST7735R Gamma Sequence
    //     sendCommand(Command::GAMCTRP1, 0x0F_U8, 0x1A_U8, 0x0F_U8, 0x18_U8, 0x2F_U8, 0x28_U8, 0x20_U8);
    //     sendData(0x22_U8, 0x1F_U8, 0x1B_U8, 0x23_U8, 0x37_U8, 0x00_U8, 0x07_U8);
    //     sendData(0x02_U8, 0x10_U8);
    //     sendCommand(Command::GAMCTRN1, 0x0F_U8, 0x1B_U8, 0x0F_U8, 0x17_U8, 0x33_U8, 0x2C_U8, 0x29_U8);
    //     sendData(0x2E_U8, 0x30_U8, 0x30_U8, 0x39_U8, 0x3F_U8, 0x00_U8, 0x07_U8);
    //     sendData(0x03_U8, 0x10_U8);

    //     sendCommand(Command::CASET, 0x00_U8, 0x00_U8, 0x00_U8, 0x7F_U8);
    //     sendCommand(Command::RASET, 0x00_U8, 0x00_U8, 0x00_U8, 0x7F_U8);

    //     sendCommand(Command::COLMOD, 0x05_U8);
    //     sendCommand(Command::DISPON);  // Display on
        
    //     HAL_GPIO_WritePin(SCREEN_BL_GPIO_Port, SCREEN_BL_Pin, GPIO_PIN_SET);

    //     return true;
    // }

    // 设置绘制窗口
    void setWindow(Point ul, Point lr) {
        sendCommand(
            Command::CASET, static_cast<std::uint8_t>(ul.x >> 8), static_cast<std::uint8_t>(ul.x & 0xFFU),
            static_cast<std::uint8_t>(lr.x >> 8), static_cast<std::uint8_t>(lr.x & 0xFFU));
        sendCommand(
            Command::RASET, static_cast<std::uint8_t>(ul.y >> 8), static_cast<std::uint8_t>(ul.y & 0xFFU),
            static_cast<std::uint8_t>(lr.y >> 8), static_cast<std::uint8_t>(lr.y & 0xFFU));
    }

    void setPixel(Point pos, Point diagonal, std::span<Color> data) {
        auto result = clipRect(pos, diagonal);
        if (!result) return;
        auto [start, end] = *result;
        diagonal = end - start;
        auto size = static_cast<std::size_t>((diagonal.x + 1) * (diagonal.y + 1));
        if (size != data.size()) return;
        setWindow(start, end);
        sendCommand(Command::RAMWR, data);
    }

    void setPixel(Point pos, Point diagonal, Color color) {
        // 规范化矩形
        auto result = clipRect(pos, diagonal);
        if (!result) return;
        auto [start, end] = *result;
        diagonal = end - start;
        // 计算绘制区域大小
        auto size = static_cast<std::size_t>((diagonal.x + 1) * (diagonal.y + 1));
        setWindow(start, end);
        // 填充缓冲区
        if (size <= _DT::PIXEL_COUNT) std::fill_n(framebuffer, size, color);
        else std::fill_n(framebuffer, _DT::PIXEL_COUNT, color);
        // 发送写显存命令
        sendCommand(Command::RAMWR);
        // 发送缓冲区，绘制区域过大则分批发送
        while (size > _DT::PIXEL_COUNT) {
            sendData(std::span{framebuffer});
            size -= _DT::PIXEL_COUNT;
        }
        // 发送剩余内容
        sendData(std::span{framebuffer, size});
    }

    void setPixel(Point pos, Color color) {
        Point diagonal{0, 0};
        auto result = clipRect(pos, diagonal);
        if (!result) return;
        auto [start, end] = *result;
        diagonal = end - start;
        setWindow(start, end);
        sendCommand(
            Command::RAMWR, static_cast<std::uint8_t>(color.value & 0xFF), static_cast<std::uint8_t>(color.value >> 8));
    }



    void wait() {
        // SpiDmaEngine::getInstance().wait();
    }
};
