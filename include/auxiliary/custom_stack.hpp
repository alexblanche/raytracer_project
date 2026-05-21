#pragma once

#include <vector>
#include <cstring>

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

    inline void push(const T* const ts, unsigned int size) {
        v.reserve(v.size() + size);
        std::memcpy(v.data() + v.size(), ts, size);
    }

    inline void push(const std::vector<T>& ts) {
        push(ts.data(), ts.size());
    }

    inline void set_empty() {
        v.clear();
    }
};