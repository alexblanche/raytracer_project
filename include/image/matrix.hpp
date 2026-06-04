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

        matrix() {}

        matrix(int width, int height) 
            : data(width, std::vector<rt::color>(height)),
              width(width), height(height) {}

        matrix(matrix&&)                 = default;
        matrix& operator=(matrix&&)      = default;
        matrix(const matrix&)            = delete;
        matrix& operator=(const matrix&) = delete;

        dimensions get_dimensions() const {
            return { width, height };
        }

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

        const rt::color& get(int col, int row) const {
            return data[col][row];
        }

        rt::color& get(int col, int row) {
            return data[col][row];
        }

        const rt::color& operator[](int col, int row) const {
            return get(col, row);
        }

        rt::color& operator[](int col, int row) {
            return get(col, row);
        }

        void apply_gamma(real gamma);
};