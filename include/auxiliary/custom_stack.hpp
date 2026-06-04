#pragma once

#include <cstring>
#include <span>

template <typename T>
requires (sizeof(T) <= 16) && std::is_trivially_destructible_v<T>
class custom_stack {

    T*  data     = nullptr;
    int size     = 0;
    int capacity = 0;

    private:
        
        inline void increase_capacity(int target) {
            T* new_data = new T[target];
            std::memcpy(new_data, data, size * sizeof(T));
            data = new_data;
            capacity = target;
        }

        inline void check_capacity() {
            if (size >= capacity)
                increase_capacity(2 * capacity);
        }

        inline void reserve(int target) {
            if (capacity < target)
                increase_capacity(std::max(target, 2 * capacity));
        }

    public:

        inline custom_stack(unsigned int init_size = 10) {
            size = 0;
            capacity = init_size;
            data = new T[capacity];
        }

        inline ~custom_stack() noexcept {
            delete[] data;
            size = 0;
            capacity = 0;
        }

        int get_size() const {
            return size;
        }

        // int get_capacity() const {
        //     return capacity;
        // }

        std::span<const T> get_content() const {
            return { data, static_cast<size_t>(size) };
        }

        [[nodiscard]] inline bool empty() const noexcept {
            return size == 0;
        }

        inline T pop() {
            size--;
            return data[size];
        }

        [[nodiscard]] inline const T& top() const {
            return data[size - 1];
        }

        [[nodiscard]] inline T& top() {
            return data[size - 1];
        }

        template<typename... Args>
        requires (requires (Args... args) { T(args...); })
        inline void emplace(Args&&... args) {
            check_capacity();
            data[size] = T(std::forward<Args>(args)...);
            size++;
        }

        inline void push(const T t) {
            emplace(t);
        }

        inline void push(const std::span<const T> ts) {
            const int new_size = size + ts.size();
            reserve(new_size);
            std::memcpy(data + size, ts.data(), ts.size() * sizeof(T));
            size = new_size;
        }

        inline void set_empty() {
            size = 0;
        }
};