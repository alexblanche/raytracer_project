#pragma once

#include <string>
#include <optional>

/* Struct containing a material or texture, its name and the index objects are going to store */

template <typename T>
class wrapper {
    public:
        T content;
        std::optional<std::string> name;
        size_t index;

        static inline size_t counter = 0;

        wrapper(T&& t, const std::string& name)
            : content(std::forward<T>(t)), name(name), index(counter) {

            counter ++;
        }

        wrapper(const T& t, const std::string& name)
            : content(t), name(name), index(counter) {

            counter ++;
        }

        wrapper(T&& t)
            : content(std::forward<T>(t)), name(std::nullopt), index(counter) {

            counter ++;
        }

        /* Initialization of the counter */
        static void init() {
            counter = 0;
        }

        wrapper(wrapper&&)                  = default;
        wrapper& operator=(wrapper&&)       = default;

        wrapper(const wrapper&)             = delete;
        wrapper& operator=(const wrapper&)  = delete;
};
