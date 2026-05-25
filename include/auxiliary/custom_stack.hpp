#pragma once

#include <vector>
#include <cstring>
#include <span>

template <typename T>
struct custom_stack {

    std::vector<T> v;

    custom_stack(unsigned int init_size) {
        v.reserve(init_size);
    }

    [[nodiscard]] inline bool empty() const {
        return v.empty();
    }

    inline void pop() {
        v.pop_back();
    }

    inline const T& top() const {
        return v.back();
    }

    inline void push(const T t) {
        v.push_back(t);
    }

    inline void push(const std::span<const T> ts) {
        v.reserve(v.size() + ts.size());
        std::memcpy(v.data() + v.size(), ts.data(), ts.size());
    }

    inline void set_empty() {
        v.clear();
    }
};