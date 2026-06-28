#pragma once

#include "parameters.hpp"
#include "light/vector.hpp"

struct min_max_coord {
    real min_x, max_x, min_y, max_y, min_z, max_z;

    void update_min_max_coord(rt::vector& min, rt::vector& max) {

        min = {
            std::min(min.x, min_x),
            std::min(min.y, min_y),
            std::min(min.z, min_z)
        };

        max = {
            std::max(max.x, max_x),
            std::max(max.y, max_y),
            std::max(max.z, max_z)
        };
    }

    static constexpr rt::vector min_empty = rt::vector( infinity,  infinity,  infinity);
    static constexpr rt::vector max_empty = rt::vector(-infinity, -infinity, -infinity);
};

constexpr min_max_coord empty_set_min_max_coords = {

    .min_x =  infinity,
    .max_x = -infinity,

    .min_y =  infinity,
    .max_y = -infinity,

    .min_z =  infinity,
    .max_z = -infinity
};