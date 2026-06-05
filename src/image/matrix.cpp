#include "image/matrix.hpp"

#include "parallel/parallel.hpp"

void matrix::apply_gamma(real gamma) {

    parallel_for(height, [&] (int j) {

        const matrix::row row = get_row(j);
        for (rt::color& color : row) {
            color *= 1.0_r / 255.0_r;
            color ^= gamma;
            color *= 255.0_r;
        }
    });
}