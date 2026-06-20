#pragma once

#include "image/matrix.hpp"

class image {
    public:
        matrix data;
        const real gamma = 1.0f;
        int number_of_samples = 0;

        image(int width, int height, real gamma = 1.0f)
            : data(width, height), gamma(gamma) {}

        image(matrix&& matrix, real gamma = 1.0f)
            : data(std::move(matrix)), gamma(gamma), number_of_samples(1) {}

        image(image&&)                 = default;
        image(const image&)            = delete;
        image& operator=(image&&)      = delete;
        image& operator=(const image&) = delete;

        int width() const {
            return data.width;
        }

        int height() const {
            return data.height;
        }

        const rt::color& operator[](int row, int col) const {
            return data[row, col];
        }

        rt::color& operator[](int row, int col) {
            return data[row, col];
        }

        void increase_sample_count(int nb_samples = 1) {
            number_of_samples += nb_samples;
        }

        /* Applies gamma correction to the color data */
        void apply_gamma() {
            data.apply_gamma(gamma);
        }
};