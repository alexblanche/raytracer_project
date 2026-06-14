#pragma once

#include "screen/color.hpp"
#include "auxiliary/randomgen.hpp"
#include "image/matrix.hpp"

constexpr unsigned int LOWRES_DEFAULT_WIDTH  = 854;
constexpr unsigned int LOWRES_DEFAULT_HEIGHT = 480;

// alias_table needs to use double for random number generation
// real is fine for other calculations

struct alias_bin {
    real p = 0.0_r;
    unsigned int alias = 0;
};

struct light_map_sample {
    unsigned int x, y;
};

struct alias_table {
    std::vector<alias_bin> bins;
    double nb_bins; // Pre-computed double cast for sampling
    unsigned int map_width;
    unsigned int map_height;
    unsigned int pt_width;
    // unsigned int pt_height;
    real ratio_x;
    real ratio_y;

    // Constructs an alias table from a probability table
    // The width and height of the original light map and of the low res image are stored inside the alias table
    alias_table(const std::vector<real>& prob_table,
        const unsigned int map_width,
        const unsigned int map_height,
        const unsigned int pt_width,
        const unsigned int pt_height
    );

    alias_table(const matrix& matrix,
        const unsigned int pt_width,
        const unsigned int pt_height)

    : alias_table(compute_low_res_table(matrix), matrix.width, matrix.height, pt_width, pt_height) {}

    // Should be called in each thread at initialization
    static inline random_ratio_gen<double> get_random_generator() {
        return random_ratio_gen<double>();
    }

    inline unsigned int sample_table(const random_ratio_gen<double>& rg) const {

        const unsigned int i = static_cast<unsigned int>(rg.random(nb_bins));
        const auto [ p, alias ] = bins[i];
        return (rg.random() < p) ? i : alias;
    }

    light_map_sample sample_light_map(const random_ratio_gen<double>& rg) const;

    private:
        static std::vector<real> compute_low_res_table(const matrix& matrix);
};


