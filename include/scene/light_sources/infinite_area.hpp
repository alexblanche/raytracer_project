#pragma once

#include "screen/color.hpp"
#include "auxiliary/randomgen.hpp"

#include <vector>

constexpr unsigned int LOWRES_DEFAULT_WIDTH  = 854;
constexpr unsigned int LOWRES_DEFAULT_HEIGHT = 480;

struct alias_bin {
    real p = 0.0_r;
    unsigned int alias = 0;
};

struct light_map_sample {
    unsigned int x;
    unsigned int y;
};

struct alias_table {
    std::vector<alias_bin> bins;
    real nb_bins; // Used for sampling
    unsigned int map_width;
    unsigned int map_height;
    unsigned int pt_width;
    // unsigned int pt_height;
    real ratio_x;
    real ratio_y;
    
    static std::vector<real> compute_low_res_table(const std::vector<std::vector<rt::color>>& matrix);

    // Constructs an alias table from a probability table
    // The width and height of the original light map and of the low res image are stored inside the alias table
    alias_table(const std::vector<real>& prob_table,
        const unsigned int map_width,
        const unsigned int map_height,
        const unsigned int pt_width,
        const unsigned int pt_height
    );

    inline unsigned int sample(const randomgen& rg) const {
        const unsigned int i = rg.random_real(nb_bins);
        const auto& [ p, alias ] = bins[i];
        return (rg.random_ratio() < p) ? i : alias;
    }

    light_map_sample get_sample_for_light_map(const randomgen& rg) const;
};


