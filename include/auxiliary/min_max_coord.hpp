#pragma once

#include "parameters.hpp"
#include "light/vector.hpp"

#include <vector>

struct min_max_coord {
    real min_x, max_x, min_y, max_y, min_z, max_z;

    void update(rt::vector& min, rt::vector& max) const {

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

constexpr min_max_coord build_min_max_coord(const rt::vector& min, const rt::vector& max) {
    return {
        .min_x = min.x,
        .max_x = max.x,

        .min_y = min.y,
        .max_y = max.y,

        .min_z = min.z,
        .max_z = max.z
    };
}

/* Returns two vectors [ min, max ]
   The set of objects are contained within min and min + max */
template<typename T>
requires (requires (T x) { { x.get_min_max_coord() } -> std::same_as<min_max_coord>; })
std::pair<rt::vector, rt::vector> compute_bounding_vectors(const std::vector<const T*>& set) {
    
    rt::vector max = min_max_coord::max_empty;
    rt::vector min = min_max_coord::min_empty;

    for (const T* p : set)
        p->get_min_max_coord().update(min, max);
    
    return { min, max };
}