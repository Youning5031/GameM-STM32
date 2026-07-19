#pragma once

#include <cstdint>

struct Point {
    using pType = std::int16_t;
    pType x, y;

    constexpr bool operator==(const Point &) const noexcept = default;

    constexpr Point &operator+=(const Point &rhs) noexcept {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    constexpr Point &operator-=(const Point &rhs) noexcept {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    constexpr Point &operator*=(const pType scalar) noexcept {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    constexpr Point &operator*=(const Point &rhs) noexcept {
        x *= rhs.x;
        y *= rhs.y;
        return *this;
    }

    pType dot(const Point &rhs) const noexcept {
        return x * rhs.x + y * rhs.y;
    }

    pType cross(const Point &rhs) const noexcept {
        return x * rhs.y - y * rhs.x;
    }
};

constexpr Point operator+(Point lhs, const Point &rhs) noexcept {
    lhs += rhs;
    return lhs;
}

constexpr Point operator-(Point lhs, const Point &rhs) noexcept {
    lhs -= rhs;
    return lhs;
}

constexpr Point operator*(Point lhs, const Point &rhs) noexcept {
    lhs *= rhs;
    return lhs;
}

constexpr Point operator*(Point::pType lhs, const Point &rhs) noexcept {
    auto ret = rhs;
    ret*=rhs;
    return ret;
}

constexpr Point operator*(Point lhs, Point::pType rhs) noexcept {
    lhs *= rhs;
    return lhs;
}
