#pragma once

#include "image/matrix.hpp"

class image {
    public:
        matrix data;
        const real gamma = 1.0f;
        unsigned int number_of_samples = 0;

        image(int width, int height, real gamma = 1.0f)
            : data(width, height), gamma(gamma) {}

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

        void increase_sample_count(int nb_samples = 1) {
            number_of_samples += nb_samples;
        }

        /* Applies gamma correction to the color data */
        void apply_gamma() {
            data.apply_gamma(gamma);
        }
};