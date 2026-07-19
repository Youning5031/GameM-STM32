#pragma once

#include "magic_enum\magic_enum.hpp"
#include "Point.hpp"

#include <cstddef>
#include <ranges>

struct Block {
    enum class Type : std::uint8_t {
        I,
        O,
        T,
        S,
        Z,
        J,
        L,
        NONE,
    };

    enum class State {
        ROT_0,    // 0°
        ROT_90,   // 90°
        ROT_180,  // 180°
        ROT_270,  // 270°
    };

    // clang-format off

    // 预计算的方块数组，储存方块各小块相对于局部网格左上角的坐标。
    // I型方块网格为4*4大小，其余为3*3大小
    constexpr const static Point BLOCKS[magic_enum::enum_count<Type>() - 1][magic_enum::enum_count<State>()][4] = {
        // I Shape
        {
            {{0, 1}, {1, 1}, {2, 1}, {3, 1}, },
            {{1, 3}, {1, 2}, {1, 1}, {1, 0}, },
            {{3, 2}, {2, 2}, {1, 2}, {0, 2}, },
            {{2, 0}, {2, 1}, {2, 2}, {2, 3}, },
        },
        // O Shape
        {
            {{0, 0}, {1, 0}, {0, 1}, {1, 1}, },
            {{0, 0}, {1, 0}, {0, 1}, {1, 1}, },
            {{0, 0}, {1, 0}, {0, 1}, {1, 1}, },
            {{0, 0}, {1, 0}, {0, 1}, {1, 1}, },
        },
        // T Shape
        {
            {{1, 0}, {0, 1}, {1, 1}, {2, 1}, },
            {{0, 1}, {1, 2}, {1, 1}, {1, 0}, },
            {{1, 2}, {2, 1}, {1, 1}, {0, 1}, },
            {{2, 1}, {1, 0}, {1, 1}, {1, 2}, },
        },
        // S Shape
        {
            {{1, 0}, {2, 0}, {0, 1}, {1, 1}, },
            {{0, 1}, {0, 0}, {1, 2}, {1, 1}, },
            {{1, 2}, {0, 2}, {2, 1}, {1, 1}, },
            {{2, 1}, {2, 2}, {1, 0}, {1, 1}, },
        },
        // Z Shape
        {
            {{0, 0}, {1, 0}, {1, 1}, {2, 1}, },
            {{0, 2}, {0, 1}, {1, 1}, {1, 0}, },
            {{2, 2}, {1, 2}, {1, 1}, {0, 1}, },
            {{2, 0}, {2, 1}, {1, 1}, {1, 2}, },
        },
        // J Shape
        {
            {{1, 0}, {1, 1}, {0, 2}, {1, 2}, },
            {{0, 1}, {1, 1}, {2, 2}, {2, 1}, },
            {{1, 2}, {1, 1}, {2, 0}, {1, 0}, },
            {{2, 1}, {1, 1}, {0, 0}, {0, 1}, },
        },
        // L Shape
        {
            {{1, 0}, {1, 1}, {1, 2}, {2, 2}, },
            {{0, 1}, {1, 1}, {2, 1}, {2, 0}, },
            {{1, 2}, {1, 1}, {1, 0}, {0, 0}, },
            {{2, 1}, {1, 1}, {0, 1}, {0, 2}, },
        },
    };
    // clang-format on
    Type type;
    State state;
    Point pos;

    Block() : type(Type::NONE), state(State::ROT_0), pos(0, 0) {};
    Block(Type type, Point pos) : type(type), state(State::ROT_0), pos(pos) {}
    Block(const Block &other) : type(other.type), state(other.state), pos(other.pos) {}

    void rotateLeft() {
        switch (state) {
        case State::ROT_0: state = State::ROT_270; break;
        case State::ROT_90: state = State::ROT_0; break;
        case State::ROT_180: state = State::ROT_90; break;
        case State::ROT_270: state = State::ROT_180; break;
        }
    }

    void rotateRight() {
        switch (state) {
        case State::ROT_0: state = State::ROT_90; break;
        case State::ROT_90: state = State::ROT_180; break;
        case State::ROT_180: state = State::ROT_270; break;
        case State::ROT_270: state = State::ROT_0; break;
        }
    }

    void moveLeft() {
        pos.x--;
    }
    void moveRight() {
        pos.x++;
    }
    void moveDown() {
        pos.y++;
    }
    struct Iterator {
    private:
        const Block *parent;
        std::size_t idx;

    public:
        using iterator_concept = std::input_iterator_tag;  // 因为按值返回，属于输入迭代器
        using value_type = Point;
        using difference_type = std::ptrdiff_t;
        using pointer = Point *;
        using reference = Point;

        explicit Iterator(const Block *p = nullptr, std::size_t i = 0) : parent(p), idx(i) {}

        // 每次解引用都实时计算
        reference operator*() const {
            return parent->BLOCKS[static_cast<std::size_t>(parent->type)][static_cast<std::size_t>(parent->state)][idx]
                 + parent->pos;
        }

        reference operator->() const {
            return parent->BLOCKS[static_cast<std::size_t>(parent->type)][static_cast<std::size_t>(parent->state)][idx]
                 + parent->pos;
        }

        Iterator &operator++() {
            ++idx;
            return *this;
        }

        Iterator operator++(int) {
            auto tmp = *this;
            ++idx;
            return tmp;
        }

        bool operator==(const Iterator &other) const {
            return idx == other.idx;
        }
    };

    Iterator begin() {
        return Iterator(this, 0);
    }
    Iterator end() {
        return Iterator(this, 4);
    }
    Iterator begin() const {
        return Iterator(this, 0);
    }
    Iterator end() const {
        return Iterator(this, 4);
    }
};

static_assert(std::ranges::range<Block>, "Block 不可遍历");
