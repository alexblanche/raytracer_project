#pragma once

#include <vector>
#include "screen/color.hpp"
#include "auxiliary/randomgen.hpp"

constexpr unsigned int LOWRES_DEFAULT_WIDTH = 854;
constexpr unsigned int LOWRES_DEFAULT_HEIGHT = 480;

std::vector<real> compute_low_res_table(std::vector<std::vector<rt::color>>& matrix);

struct alias_bin {
    real p;
    unsigned int alias;

    alias_bin() : p(0.0f), alias(0) {}
    alias_bin(const real p, const unsigned int alias)
        : p(p), alias(alias) {}
};

struct alias_table {
    std::vector<alias_bin> bins;

    unsigned int map_width;
    unsigned int map_height;
    unsigned int pt_width;
    // unsigned int pt_height;
    real ratio_x;
    real ratio_y;
    
    // Constructs an alias table from a probability table
    // The width and height of the original light map and of the low res image are stored inside the alias table
    alias_table(const std::vector<real> prob_table,
        const unsigned int map_width,
        const unsigned int map_height,
        const unsigned int pt_width,
        const unsigned int pt_height
    );

    unsigned int sample(randomgen& rg);

    std::pair<unsigned int, unsigned int> get_sample_for_light_map(randomgen& rg);
};


