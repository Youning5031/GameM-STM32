#include "SpiDmaEngine.hpp"

#include "stm32f1xx_hal_spi.h"

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    SpiDmaEngine::getInstance().callback();
}
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
    SpiDmaEngine::getInstance().callback();
}
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
    SpiDmaEngine::getInstance().callback();
}
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
    SpiDmaEngine::getInstance().errorCallback();
}
