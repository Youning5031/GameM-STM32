#include <cstdint>
#include <limits>

// 没有回绕检查
// consteval std::uint8_t operator""_U8(unsigned long long value) {
//     return static_cast<std::uint8_t>(value);
// }

template<char C>
consteval unsigned long long charToInt() {
    if constexpr ('0' <= C && C <= '9') return C - '0';
    else if constexpr ('a' <= C && C <= 'f') return C - 'a' + 10;
    else if constexpr ('A' <= C && C <= 'F') return C - 'A' + 10;
    else {
        static_assert(C == '\'', "不是数字");
        return 0;
    }
}

template<int Base = 10, char... Cs>
struct ParseInt {
    unsigned long long val{0};
    consteval ParseInt() {
        ((val = (Cs != '\'' ? (val * Base + charToInt<Cs>()) : val)), ...);
    }
};

template<char... C>
struct LiteralToInt {
    static constexpr unsigned long long value = ParseInt<10, C...>().val;
};

template<char... Rest>
struct LiteralToInt<'0', 'b', Rest...> {
    static constexpr unsigned long long value = ParseInt<2, Rest...>().val;
};

template<char... Rest>
struct LiteralToInt<'0', 'B', Rest...> {
    static constexpr unsigned long long value = ParseInt<2, Rest...>().val;
};

template<char... Rest>
struct LiteralToInt<'0', 'x', Rest...> {
    static constexpr unsigned long long value = ParseInt<16, Rest...>().val;
};

template<char... Rest>
struct LiteralToInt<'0', 'X', Rest...> {
    static constexpr unsigned long long value = ParseInt<16, Rest...>().val;
};

template<char... Rest>
struct LiteralToInt<'0', Rest...> {
    static constexpr unsigned long long value = ParseInt<8, Rest...>().val;
};

template<>
struct LiteralToInt<'0'> {
    static constexpr unsigned long long value = 0;
};

template<char... Cs>
constexpr std::uint8_t operator""_U8() {
    constexpr auto val = LiteralToInt<Cs...>::value;
    static_assert(
        val <= std::numeric_limits<std::uint8_t>::max(), "数值超出 uint8_t 范围(0~255)，请使用 static_cast。");
    return static_cast<std::uint8_t>(val);
}
