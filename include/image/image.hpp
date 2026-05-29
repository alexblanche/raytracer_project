#pragma once

#include "image/matrix.hpp"

class image {
    public:
        matrix matrix;
        const real gamma = 1.0f;
        unsigned int number_of_samples = 0;

        image(unsigned int width, unsigned int height, real gamma = 1.0f)
            : matrix(width, height), gamma(gamma) {}

        image(image&&)                 = delete;
        image(const image&)            = delete;
        image& operator=(image&&)      = delete;
        image& operator=(const image&) = delete;

        void increase() {
            number_of_samples++;
        }
};