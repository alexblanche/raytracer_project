#pragma once

#include <cstring>
#include <span>
#include <memory>

namespace stack_based {
    inline static constexpr std::size_t DEFAULT_INIT_SIZE = 10; // Default initial reserve
    inline static constexpr std::size_t MAX_ELEMENT_SIZE  = 16; // Maximum size of stored elements
}

enum class allocation_type {
    Static, Dynamic
};

template <
    typename T,
    std::size_t first_buffer_size = stack_based::DEFAULT_INIT_SIZE,
    allocation_type allocation_type_ = allocation_type::Dynamic
>
requires (sizeof(T) <= stack_based::MAX_ELEMENT_SIZE) // Not so important, can be lifted if needed
    && std::is_trivially_copy_constructible_v<T> // Because of std::memcpy for push of span
    && std::is_trivially_destructible_v<T>
class stack_based_custom_stack {

    using enum allocation_type;

    inline static std::allocator<T> allocator;
    using alloc = std::allocator_traits<std::allocator<T>>;

    T first_buffer[first_buffer_size];
    T* data              = nullptr;
    std::size_t size     = 0;
    std::size_t capacity = 0;
    bool dynamic_buffer_allocated = false;

    private:
        
        inline void increase_capacity(const std::size_t target) {
            T* new_data(alloc::allocate(allocator, target));
            std::memcpy(reinterpret_cast<void*>(new_data), reinterpret_cast<void*>(data), size * sizeof(T));
            if (dynamic_buffer_allocated)
                alloc::deallocate(allocator, data, capacity);
            dynamic_buffer_allocated = true;
            data = new_data;
            capacity = target;
        }

        inline void check_capacity() {
            if (size >= capacity) {
                if constexpr (allocation_type_ == Static) {
                    throw std::runtime_error("Static stack-based stack capacity exceeded");
                }

                increase_capacity(2 * capacity);
            }
        }

        inline void reserve(const std::size_t target) {
            if (capacity < target) {
                if constexpr (allocation_type_ == Static) {
                    throw std::runtime_error("Static stack-based stack capacity exceeded");
                }
                increase_capacity(std::max(target, 2 * capacity));
            }
        }

    public:

        inline stack_based_custom_stack(const std::size_t init_size = stack_based::DEFAULT_INIT_SIZE) {
            size     = 0;
            capacity = init_size;
            if (init_size > first_buffer_size) {
                if constexpr (allocation_type_ == Static)
                    throw std::runtime_error("Static stack-based stack capacity exceeded");
                dynamic_buffer_allocated = true;
                data = alloc::allocate(allocator, capacity);
            }
            else {
                data     = first_buffer;
                capacity = first_buffer_size;
            }
        }

        inline ~stack_based_custom_stack() noexcept {
            if (dynamic_buffer_allocated)
                alloc::deallocate(allocator, data, capacity);
            size     = 0;
            capacity = 0;
        }

        std::size_t get_size() const {
            return size;
        }

        std::size_t get_capacity() const {
            return capacity;
        }

        std::span<const T> get_content() const {
            return { data, size };
        }

        [[nodiscard]] inline bool empty() const noexcept {
            return size == 0;
        }

        inline T pop() {
            size--;
            if constexpr (allocation_type_ == Dynamic) {
                return data[size];
            }
            else {
                return first_buffer[size];
            }
        }

        [[nodiscard]] inline const T& top() const {
            if constexpr (allocation_type_ == Dynamic) {
                return data[size - 1];
            }
            else {
                return first_buffer[size - 1];
            }
        }

        [[nodiscard]] inline T& top() {
            if constexpr (allocation_type_ == Dynamic) {
                return data[size - 1];
            }
            else {
                return first_buffer[size - 1];
            }
        }

        template<typename... Args>
        requires std::is_constructible_v<T, Args...>
        inline void emplace(Args&&... args) {
            check_capacity();

            if constexpr (allocation_type_ == Static) {
                first_buffer[size] = T(std::forward<Args>(args)...);
            }
            else {
                if (dynamic_buffer_allocated)
                    alloc::construct(allocator, &data[size], std::forward<Args>(args)...);
                else
                    first_buffer[size] = T(std::forward<Args>(args)...);
            }
            size++;
        }

        inline void push(const T t) {
            emplace(t);
        }

        inline void push(const std::span<const T> ts) {
            if (ts.size() == 0)
                return;
            const std::size_t new_size = size + ts.size();
            reserve(new_size);
            std::memcpy(data + size, ts.data(), ts.size() * sizeof(T));
            size = new_size;
        }

        inline void set_empty() {
            size = 0;
        }
};