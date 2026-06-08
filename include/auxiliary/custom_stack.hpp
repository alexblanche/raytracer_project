#pragma once

#include <cstring>
#include <span>

constexpr std::size_t DEFAULT_INIT_SIZE = 10;
constexpr std::size_t MAX_STACK_SIZE = 16;

template <typename T>
requires (sizeof(T) <= MAX_STACK_SIZE) && std::is_trivially_destructible_v<T>
class custom_stack {

    T*  data     = nullptr;
    std::size_t size     = 0;
    std::size_t capacity = 0;

    private:
        
        inline void increase_capacity(const std::size_t target) {
            T* new_data = new T[target];
            std::memcpy(new_data, data, size * sizeof(T));
            data = new_data;
            capacity = target;
        }

        inline void check_capacity() {
            if (size >= capacity)
                increase_capacity(2 * capacity);
        }

        inline void reserve(const std::size_t target) {
            if (capacity < target)
                increase_capacity(std::max(target, 2 * capacity));
        }

    public:

        inline custom_stack(const std::size_t init_size = DEFAULT_INIT_SIZE) {
            size = 0;
            capacity = init_size;
            data = new T[capacity];
        }

        inline ~custom_stack() noexcept {
            delete[] data;
            size = 0;
            capacity = 0;
        }

        std::size_t get_size() const {
            return size;
        }

        std::span<const T> get_content() const {
            return { data, size };
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
            const std::size_t new_size = size + ts.size();
            reserve(new_size);
            std::memcpy(data + size, ts.data(), ts.size() * sizeof(T));
            size = new_size;
        }

        inline void set_empty() {
            size = 0;
        }
};