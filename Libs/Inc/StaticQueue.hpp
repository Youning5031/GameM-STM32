#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <optional>

template<typename T, std::size_t Size>
class StaticQueue {
public:
    static_assert(std::has_single_bit(Size), "队列大小必须是二的幂");
    using ItemType = T;
    static constexpr auto SIZE = Size;

private:
    std::array<T, Size> buffer;
    std::size_t head{0};
    std::size_t tail{0};

public:
    std::size_t getHead() {
        return head;
    }
    std::size_t getTail() {
        return tail;
    }

    bool push(const T &item) noexcept {
        auto next = (tail + 1) % Size;
        if (next == head) return false;
        buffer[tail] = item;
        tail = next;
        return true;
    }

    bool push(T &&item) noexcept {
        auto next = (tail + 1) % Size;
        if (next == head) return false;
        buffer[tail] = std::move(item);
        tail = next;
        return true;
    }

    std::optional<T> pop() noexcept {
        if (head == tail) return std::nullopt;
        auto out = std::move(buffer[head]);
        head = (head + 1) % Size;
        return out;
    }

    T* peek() noexcept {
        if (head == tail) return nullptr;
        return &buffer[head];
    }

    bool isFull() const noexcept {
        return ((tail + 1) % Size) == head;
    }
    bool isEmpty() const noexcept {
        return head == tail;
    }
    void clear() noexcept {
        head = 0;
        tail = 0;
    }
};
