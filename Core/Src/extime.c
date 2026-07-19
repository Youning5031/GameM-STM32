#include "extime.h"

#include "cmsis_gcc.h"
#include "main.h"
#include "tim.h"

#include <stdint.h>


int64_t timestamp = 0;

void button_Systick_callback();

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        timestamp++;
    }
}

void HAL_SYSTICK_Callback() {
    if (HAL_GetTick() % 8 == 0) {
        button_Systick_callback();
    }
}

void delayMillis(size_t ms) {
    if (ms == 0) return;
    // 每次延时 64ms，避免单次调用超过 65.535ms 的 16 位定时器限制
    for (; ms >= 64; ms -= 64) {
        delayMicros(64000);
    }
    // 处理剩余的毫秒部分
    if (ms > 0) {
        delayMicros(ms * 1000);
    }
}

void delayMicros(size_t us) {
    if (us == 0) return;
    uint32_t start = timeMicros();
    while (timeMicros() - start < us) {
        __NOP();
    }
}

int64_t timeMicros() {
    int32_t th1, th2;
    uint32_t tl;
    do {
        th1 = timestamp;
        tl = timebase.Instance->CNT;
        th2 = timestamp;
    } while (th1 != th2);

    return ((int64_t)th1 * 1000) + (int64_t)(tl & 0xFFFF);
}

int64_t timeMillis() {
    return timestamp;
}