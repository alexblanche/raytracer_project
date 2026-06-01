#include "image/matrix.hpp"

#include "parallel/parallel.hpp"

void matrix::apply_gamma(real gamma) {

    parallel_for(width, [&] (int i) {

        for (rt::color& col : data[i]) {
            col *= static_cast<real>(1.0f / 255.0f);
            col ^= gamma;
            col *= static_cast<real>(255.0f);
        }
    });
}