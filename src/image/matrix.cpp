#include "image/matrix.hpp"

#include "parallel/parallel.hpp"

void matrix::apply_gamma(real gamma) {

    parallel_for(height, [&] (int j) {

        for (rt::color& color : data[j]) {
            color *= 1.0_r / 255.0_r;
            color ^= gamma;
            color *= 255.0_r;
        }
    });
}