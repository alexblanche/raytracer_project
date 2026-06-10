#include "image/matrix.hpp"

#include "parallel/parallel.hpp"

void matrix::apply_gamma(real gamma) {

    parallel_for(height, [&] (int j) {

        const matrix::row row = get_row(j);
        for (rt::color& color : row) {
            color.apply_gamma(gamma);
        }
    });
}