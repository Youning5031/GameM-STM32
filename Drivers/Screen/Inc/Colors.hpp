#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>

template<typename T>
concept Colors = requires {
    // 检查是否存在打包字段
    T::IS_PACKED;
    requires std::is_same_v<decltype(T::IS_PACKED), const bool>;
    // 检查像素位宽，位宽需要大于0，且防止有多余成员
    { T::PIXEL_WIDTH } -> std::convertible_to<int>;
    requires T::PIXEL_WIDTH <= sizeof(T) * 8;
    requires T::PIXEL_WIDTH > (sizeof(T) - 1) * 8;
    requires T::PIXEL_WIDTH > 0;
};

#define colors_static_assert(type) static_assert(Colors<type>, "类型 " #type " 不是颜色")

struct RGB565 {
public:
    std::uint16_t value;

    constexpr static bool IS_PACKED = false;
    constexpr static std::size_t PIXEL_WIDTH = 16;

    constexpr RGB565(std::uint16_t value) : value(__builtin_bswap16(value)) {};
    constexpr RGB565(std::uint8_t r, std::uint8_t g, std::uint8_t b) : RGB565((r << 11) | (g << 5) | b) {};
    constexpr RGB565() : value(0x0000U) {};

    constexpr std::uint8_t r() const {
        return (value >> 11) & 0x1FU;
    }
    constexpr std::uint8_t g() const {
        return (value >> 5) & 0x3FU;
    }
    constexpr std::uint8_t b() const {
        return value & 0x1FU;
    }
    constexpr bool operator==(const RGB565 &other) const {
        return value == other.value;
    }
};

colors_static_assert(RGB565);

constexpr inline RGB565 WHITE{0xFFFFU};
constexpr inline RGB565 BLACK{0x0000U};
constexpr inline RGB565 RED{0xF800U};
constexpr inline RGB565 GREEN{0x07E0U};
constexpr inline RGB565 BLUE{0x001FU};
constexpr inline RGB565 CYAN(0x07FF);
constexpr inline RGB565 YELLOW(0xFFE0);
constexpr inline RGB565 PURPLE(0xE01F);
constexpr inline RGB565 ORANGE(0xFD20);
