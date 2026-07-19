#include "Button.hpp"

#include "main.h"
#include "stm32f1xx_hal_gpio.h"

#include <array>
#include <ranges>

#define CREATE_BUTTON(name)                            \
    Button {                                           \
        BUTTON_##name##_GPIO_Port, BUTTON_##name##_Pin \
    }

struct ButtonManager {
    inline static auto buttons = std::array{
        CREATE_BUTTON(up), CREATE_BUTTON(down), CREATE_BUTTON(left), CREATE_BUTTON(right),
        CREATE_BUTTON(a),  CREATE_BUTTON(b),    Button{nullptr, 0},
    };
};

Button &Button::getInstance(Port port, Pin pin) {
    for (auto &&b : ButtonManager::buttons | std::views::take(ButtonManager::buttons.size() - 1)) {
        if (b.port == port && b.pin == pin) return b;
    }
    return ButtonManager::buttons.back();
}

// 消抖扫描
void Button::scanButton() {
    for (auto &&b : ButtonManager::buttons | std::views::take(ButtonManager::buttons.size() - 1)) {
        auto current_state = HAL_GPIO_ReadPin(b.port, b.pin);
        if (b.state != current_state) b.debounce_duration += 1;
        else b.debounce_duration = 0;

        b.last_state = b.state;
        if (b.debounce_duration > 2) {  // (2 + 1) * 8ms = 24ms
            b.state = current_state;
            b.duration = 0;
        } else {
            b.duration += 1;
        }
    }
}

extern "C" void button_Systick_callback() {
    Button::scanButton();
}
