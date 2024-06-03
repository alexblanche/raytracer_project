#pragma once

#include <string>
#include <optional>

/* Struct containing a material or texture, its name and the index objects are going to store */

template <typename T>
struct wrapper {
    T content;
    std::optional<std::string> name;
    size_t index;

    static size_t counter;

    wrapper(const T& t, const std::string& name)
        : content(t), name(name), index(counter) {

        counter ++;
    }

    wrapper(const T& t)
        : content(t), name(std::nullopt), index(counter) {

        counter ++;
    }

    /* Initialization of the counter */
    static void init() {
        counter = 0;
    }
};

/* The counters are declared in scene_parser.cpp, and initialized at the beginning of each scene parsing */
