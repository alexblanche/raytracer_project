#pragma once

#include <vector>

template <typename T>
struct custom_stack {

    std::vector<T> v;

    custom_stack(const unsigned int init_size) {
        v.reserve(init_size);
    }

    inline bool empty() const {
        return v.size() == 0;
    }

    inline void pop() {
        v.pop_back();
    }

    inline const T& top() const {
        return v.back();
    }

    void push(T t) {
        v.push_back(t);
    }

    inline void set_empty() {
        v.resize(0);
    }

};