#pragma once

#include "Colors.hpp"
#include "font.hpp"
#include "Point.hpp"

#include <concepts>
#include <cstddef>
#include <cstring>
#include <ranges>
#include <span>
#include <utility>

// 显示特性
template<Colors ColorType, std::size_t Width, std::size_t Heigth, std::size_t BufferSize>
struct DisplayTraits {
private:
    template<std::convertible_to<std::size_t>... SimplePackedSize>
    static consteval bool inSizeList(SimplePackedSize... sizes) {
        return ((PIXEL_WIDTH == sizes) || ...);
    }

public:
    static constexpr auto WIDTH = Width;
    static constexpr auto HEIGHT = Heigth;
    static constexpr auto IS_PACKED = ColorType::IS_PACKED;
    static constexpr auto PIXEL_WIDTH = ColorType::PIXEL_WIDTH;
    static constexpr auto BUFFER_SIZE = BufferSize;
    static constexpr auto PIXEL_COUNT = BufferSize / sizeof(ColorType);
    static constexpr auto IS_SIMPLE_PACKED = inSizeList(8, 16, 32) || !IS_PACKED;
    // 屏幕缓冲区设计
    // static constexpr std::size_t STORAGE_SIZE = IS_SIMPLE_PACKED ? ((W * H * PIXEL_WIDTH + 7) / 8) : (W * H);
    // using BufferType = std::conditional_t<ColorType::IS_PACKED, std::uint8_t[STORAGE_SIZE],
    // ColorType[Height][Width]>;

    // 屏幕通信缓冲区设计
    using BufferType = ColorType[PIXEL_COUNT];
    using Color = ColorType;
};

// 概念：屏幕驱动
template<typename T>
concept ScreenDriver = requires(T t) {
    // 存在颜色类型
    typename T::Color;
    // 存在宽和高
    T::WIDTH;
    T::HEIGHT;
    // 存在缓冲区类型
    typename T::BufferType;
    // 可以初始化
    { t.init() } -> std::same_as<bool>;
    // 可以绘制像素
    { t.setPixel(std::declval<Point>(), std::declval<typename T::Color>()) };
};

// 概念：可以多像素绘制
template<typename T>
concept MultiPixelDrawable = ScreenDriver<T> && requires(T t) {
    // 一次绘制多个像素
    { t.setPixel(std::declval<Point>(), std::declval<Point>(), std::declval<typename T::Color>()) };
};

#define screen_driver_static_assert(type) static_assert(ScreenDriver<type>, "类型 " #type " 不是屏幕设备")

template<typename _SD>
class Screen {
public:
    using Controller = _SD;
    using Color = Controller::Color;
    using BufferType = Controller::BufferType;
    constexpr static auto WIDTH = Controller::WIDTH;
    constexpr static auto HEIGHT = Controller::HEIGHT;

    screen_driver_static_assert(_SD);

private:
    Controller controller;

public:
    Screen()
        requires ScreenDriver<_SD>
    = default;

    bool init() {
        return controller.init();
    };

    // 绘制实心矩形，pos为矩形左上角，diagonal为矩形对角线，如果diagonal两个分量为0，绘制一个点
    void drawRect(Point pos, Point diagonal, Color color) {
        controller.setPixel(pos, diagonal, color);
    }

    // 清屏
    void clear(Color color) {
        drawRect(Point{0, 0}, Point{WIDTH - 1, HEIGHT - 1}, color);
        controller.wait();
    }

    // 绘制连接pos1到pos2的直线，可以是斜线
    void drawLine(Point pos1, Point pos2, Color color);
    // 绘制字符c在pos处，pos为c的左上角坐标，必须给出前景色和背景色
    void drawChar(Point pos, char c, Color fcolor, Color bcolor);
    // 绘制字符串，不会自动换行
    void drawString(Point pos, const char *str, Color fcolor, Color bcolor);

    // 无实际作用，wait为空
    void flush() {
        controller.wait();
    }

private:
    void _drawChar(std::span<Color, CHAR_SIZE>, char c, Color fcolor, Color bcolor);
};

template<typename _SD>
void Screen<_SD>::drawLine(Point pos1, Point pos2, Color color) {
    auto dir = pos2 - pos1;
    if (dir.x == 0 || dir.y == 0) {
        controller.setPixel(pos1, dir, color);
        return;
    }

    // ----- Bresenham 初始化 -----
    dir.x = dir.x > 0 ? dir.x : -dir.x;
    dir.y = dir.y ? dir.y : -dir.y;
    Point::pType sx = dir.x > 0 ? 1 : -1;
    Point::pType sy = dir.y > 0 ? 1 : -1;
    Point::pType err = dir.x - dir.y;

    Point cur = pos1;       // 当前 Bresenham 点
    Point segStart = pos1;  // 当前水平段起点
    Point prev = pos1;      // 上一轮点
    if (dir.x >= dir.y) {   // ---------- 水平主导 ----------
        while (true) {
            // 若当前点非段起点，检查 y 是否变化（变化即段结束）
            if (cur != segStart) {
                if (cur.y != prev.y) {
                    // 绘制从 segStart 到 prev 的水平段（prev.x 为段终点 x）
                    controller.setPixel(segStart, {prev.x - segStart.x, 0}, color);
                    segStart = cur;  // 新段起点为当前点
                }
            }

            prev = cur;
            if (cur == pos2) break;

            // Bresenham 步进（允许 x 和 y 同时更新）
            Point::pType e2 = 2 * err;
            if (e2 > -dir.y) {
                err -= dir.y;
                cur.x += sx;
            }
            if (e2 < dir.x) {
                err += dir.x;
                cur.y += sy;
            }
        }

        // 绘制最后一段（从 segStart 到终点 prev）
        controller.setPixel(segStart, {prev.x - segStart.x, 0}, color);
    } else {  // ---------- 垂直主导 ----------
        while (true) {
            if (cur != segStart) {
                if (cur.x != prev.x) {
                    // 绘制从 segStart 到 prev 的垂直段
                    controller.setPixel(segStart, {0, prev.y - segStart.y}, color);
                    segStart = cur;
                }
            }

            prev = cur;
            if (cur == pos2) break;

            Point::pType e2 = 2 * err;
            if (e2 > -dir.y) {
                err -= dir.y;
                cur.x += sx;
            }
            if (e2 < dir.x) {
                err += dir.x;
                cur.y += sy;
            }
        }

        // 绘制最后一段
        controller.setPixel(segStart, {0, prev.y - segStart.y}, color);
    }
}

template<typename _SD>
void Screen<_SD>::_drawChar(std::span<Color, CHAR_SIZE> buffer, char c, Color fcolor, Color bcolor) {
    if (c <= FONTS_OFFSET || c >= FONTS_END) return;
    const auto &bitmap = fonts[c - FONTS_OFFSET];
    for (std::size_t i = 0; i < CHAR_SIZE; ++i) {
        buffer[i] = bitmap[i] ? fcolor : bcolor;
    }
}

template<typename _SD>
void Screen<_SD>::drawChar(Point pos, char c, Color fcolor, Color bcolor) {
    _drawChar(std::span{_SD::framebuffer, CHAR_SIZE}, c, fcolor, bcolor);
    controller.updateBuffer(pos, Point{FONT_WIDTH - 1, FONT_HEIGHT - 1});
}

// template<typename _SD>
// void Screen<_SD>::drawString(Point pos, const char *str, Color fcolor, Color bcolor) {
//     std::size_t i = 0;
//     for (; str[i] != '\0'; i++) {
//         _drawChar(std::span<Color, CHAR_SIZE>{&_SD::framebuffer[i * CHAR_SIZE], CHAR_SIZE}, str[i], fcolor, bcolor);
//         if (i % 4 == 3) {
//             controller.updateBuffer(pos, Point{FONT_WIDTH * 4 - 1, FONT_HEIGHT - 1});
//             pos.x += FONT_WIDTH * 4 - 1;
//         }
//     }
//     if (i % 4 != 3)
//         controller.updateBuffer(pos, Point{static_cast<Point::pType>(FONT_WIDTH * (i % 4 + 1) - 1), FONT_HEIGHT -
//         1});
// }

template<typename _SD>
void Screen<_SD>::drawString(Point pos, const char *str, Color fcolor, Color bcolor) {
    constexpr std::size_t CHAR_GROUP = _SD::PIXEL_COUNT / CHAR_SIZE;
    static_assert(std::popcount(CHAR_GROUP) == 1, "软件除法有性能损耗");
    std::size_t n = std::strlen(str);
    std::size_t tail_count = n % CHAR_GROUP;
    std::size_t tail_start = n - tail_count;

    for (std::size_t i = 0; i < n; i++) {
        char c = str[i];
        if (c <= FONTS_OFFSET || c >= FONTS_END) c = ' ';
        const auto &bitmap = fonts[str[i] - FONTS_OFFSET];
        for (std::size_t j = 0; j < FONT_HEIGHT; j++) {
            for (std::size_t k = 0; k < FONT_WIDTH; k++) {
                _SD::framebuffer
                    [j * FONT_WIDTH * (i < tail_start ? CHAR_GROUP : tail_count) + (i % CHAR_GROUP) * FONT_WIDTH + k]
                    = bitmap[j * FONT_WIDTH + k] ? fcolor : bcolor;
            }
        }
        if (i < tail_start && i % CHAR_GROUP == CHAR_GROUP - 1) {
            controller.updateBuffer(
                pos, Point{static_cast<Point::pType>(FONT_WIDTH * CHAR_GROUP - 1), FONT_HEIGHT - 1});
            pos.x += FONT_WIDTH * CHAR_GROUP;
        } else if (i == n - 1) {
            controller.updateBuffer(
                pos, Point{static_cast<Point::pType>(FONT_WIDTH * tail_count - 1), FONT_HEIGHT - 1});
        }
    }
}
