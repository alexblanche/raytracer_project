#pragma once

#include "image/matrix.hpp"

#include <optional>

class image {
    public:
        matrix data;
        std::optional<real> gamma;
        int number_of_samples = 0;

        image(int width, int height, std::optional<real> gamma = std::nullopt)
            : data(width, height), gamma(gamma) {}

        image(matrix&& matrix, std::optional<real> gamma = std::nullopt)
            : data(std::move(matrix)), gamma(gamma), number_of_samples(1) {}

        image(image&&) noexcept        = default;
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
            if (gamma.has_value())
                data.apply_gamma(gamma.value());
        }
};