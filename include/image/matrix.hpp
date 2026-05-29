#pragma once

#include "screen/color.hpp"

#include <span>
#include <vector>

// Replaces the one from bmp_reader.hpp
struct dimensions {
    int width, height;
};

class matrix {
    public:
        std::vector<std::vector<rt::color>> data;
        int width;
        int height;

        matrix(int width, int height) 
            : data(width, std::vector<rt::color>(height)),
              width(width), height(height) {}

        matrix(matrix&&)                 = delete;
        matrix(const matrix&)            = delete;
        matrix& operator=(matrix&&)      = delete;
        matrix& operator=(const matrix&) = delete;

        std::span<rt::color> column(int i) {
            return data[i];
        }

        std::span<const rt::color> column(int i) const {
            return data[i];
        }

        std::span<rt::color> operator[](int i) {
            return column(i);
        }

        std::span<const rt::color> operator[](int i) const {
            return column(i);
        }

        dimensions get_dimensions() const {
            return dimensions({ width, height });
        }
};