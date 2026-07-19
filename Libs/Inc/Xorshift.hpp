#pragma once

#include <cstdint>
#include <random>

constexpr static const std::uint32_t GOLDEN_RATIO = 0x9e3779b9U;
constexpr static const std::uint32_t NUMBER_MIX1 = 0xc2b2ae35U;
constexpr static const std::uint32_t NUMBER_MIX2 = 0x1b873593U;
constexpr static const std::uint32_t NUMBER_MIX3 = 0xcc9e2d51U;
constexpr static const std::uint32_t NUMBER_MIX4 = 0xe6546b64U;
constexpr static const std::uint32_t NUMBER_MIX5 = 0x27d4eb2fU;
constexpr static const std::uint32_t NUMBER_CORE = 0x85ebca6bU;

class Xorshift {
public:
    using result_type = std::uint32_t;

private:
    std::uint32_t state[4];

    // 种子混合器
    constexpr static std::uint32_t mixSeed(std::uint32_t x) noexcept {
        x ^= x >> 16;
        x *= NUMBER_MIX1;
        x ^= x >> 15;
        x *= NUMBER_MIX2;
        x ^= x >> 16;
        return x;
    }

public:
    constexpr void seed(std::uint32_t _seed) {
        _seed = _seed ? _seed : GOLDEN_RATIO;
        state[0] = mixSeed(_seed);
        state[1] = mixSeed(state[0] + NUMBER_MIX3);
        state[2] = mixSeed(state[0] + NUMBER_MIX4);
        state[3] = mixSeed(state[0] + NUMBER_MIX5);
    };

    consteval static std::uint32_t max() {
        return std::numeric_limits<std::uint32_t>::max();
    }

    consteval static std::uint32_t min() {
        return std::numeric_limits<std::uint32_t>::min();
    }

    /**
     * Xorshift RNGs
     * George Marsaglia
     * The Florida State University
     * https://www.jstatsoft.org/article/download/v008i14/916
     */
    std::uint32_t operator()() {
        std::uint32_t t = state[3];
        std::uint32_t u = state[0];

        t ^= t << 11;
        t ^= t >> 8;
        u ^= u >> 19;

        state[3] = state[2];
        state[2] = state[1];
        state[1] = state[0];
        state[0] = u ^ t;

        // 乘一个奇数改善低位分布
        return state[0] * NUMBER_CORE;
    }

    Xorshift(std::uint32_t _seed) {
        seed(_seed);
    };
    Xorshift() : Xorshift(GOLDEN_RATIO) {};
    ~Xorshift() = default;
};

static_assert(std::uniform_random_bit_generator<Xorshift>, "不满足URBG概念！");
