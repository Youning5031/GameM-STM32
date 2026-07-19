#pragma once

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_tim.h"

#include <cstdint>

using Port = GPIO_TypeDef *;
using Pin = decltype(GPIO_PIN_1);

extern "C" void button_Systick_callback();

class Button {
    friend struct ButtonManager;
    friend void button_Systick_callback();

public:
    static constexpr bool PULL_UP = true;

private:
    const Port port;
    const Pin pin;

    volatile std::uint16_t duration = 0;
    volatile bool state = PULL_UP;
    volatile bool last_state = PULL_UP;
    volatile std::uint8_t debounce_duration = 0;

    Button(Port port, Pin pin) : port(port), pin(pin) {}
    static void scanButton();

public:
    static Button &getInstance(Port port, Pin pin);

    bool isPressed() const {
        if constexpr (PULL_UP) {
            return !state;
        } else {
            return state;
        }
    }
    bool isReleased() const {
        return !isPressed();
    }
    bool isLongPressed(std::uint16_t threshold) const {
        return isPressed() && threshold < duration;
    }
    bool isToggled() const {
        return state != last_state;
    }
    bool isPressedEdge() const {
        return isPressed() && isToggled();
        // if constexpr (PULL_UP) {
        //     return !state && last_state;
        // } else {
        //     return state && !last_state;
        // }
    }
    bool isReleaseEdge() const {
        return isReleased() && isToggled();
        // if constexpr (PULL_UP) {
        //     return state && !last_state;
        // } else {
        //     return !state && last_state;
        // }
    }
    operator bool() const {
        return isPressed();
    }
};

#define BUTTON(name) Button::getInstance(BUTTON_##name##_GPIO_Port, BUTTON_##name##_Pin)
