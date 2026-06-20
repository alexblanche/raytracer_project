#pragma once

#include <cstring>
#include <span>
#include <memory>

constexpr std::size_t DEFAULT_INIT_SIZE = 10; // Default initial reserve
constexpr std::size_t MAX_ELEMENT_SIZE  = 16; // Maximum size of stored elements

template <typename T>
requires (sizeof(T) <= MAX_ELEMENT_SIZE) // Not so important, can be lifted if needed
    && std::is_trivially_copy_constructible_v<T> // Because of std::memcpy for push of span
    && std::is_trivially_destructible_v<T>
class custom_stack {

    inline static std::allocator<T> allocator;
    using alloc = std::allocator_traits<std::allocator<T>>;

    T*  data             = nullptr;
    std::size_t size     = 0;
    std::size_t capacity = 0;

    private:
        
        inline void increase_capacity(const std::size_t target) {
            T* new_data(alloc::allocate(allocator, target));
            std::memcpy(reinterpret_cast<void*>(new_data), reinterpret_cast<void*>(data), size * sizeof(T));
            alloc::deallocate(allocator, data, capacity);
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
            size     = 0;
            capacity = init_size;
            data     = alloc::allocate(allocator, capacity);
        }

        inline ~custom_stack() noexcept {
            alloc::deallocate(allocator, data, capacity);
            size     = 0;
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
        requires std::is_constructible_v<T, Args...>
        inline void emplace(Args&&... args) {
            check_capacity();
            alloc::construct(allocator, &data[size], std::forward<Args>(args)...);
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