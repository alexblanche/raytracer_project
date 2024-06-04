#pragma once

#include <string>
#include <optional>

/* Struct containing a material or texture, its name and the index objects are going to store */

template <typename T>
struct wrapper {
    T content;
    std::optional<std::string> name;
    size_t index;

    static inline size_t counter = 0;

    wrapper(T&& t, const std::string& name)
        : content(std::move(t)), name(name), index(counter) {

        counter ++;
    }

    wrapper(T&& t)
        : content(std::move(t)), name(std::nullopt), index(counter) {

        counter ++;
    }

    /* Initialization of the counter */
    static void init() {
        counter = 0;
    }

    wrapper(const wrapper& sc) = delete;

    wrapper& operator=(const wrapper& sc) = delete;

    /* Only move operations allowed */
    wrapper(wrapper&& sc) = default;

    wrapper& operator=(wrapper&& sc) = default;
};

/* The counters are declared in scene_parser.cpp, and initialized at the beginning of each scene parsing */
