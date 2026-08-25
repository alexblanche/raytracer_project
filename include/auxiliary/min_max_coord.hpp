#pragma once

#include "parameters.hpp"
#include "math/geometry/vector.hpp"

#include <vector>

struct min_max_vectors {
    rt::vector min = min_empty;
    rt::vector max = max_empty;

    static constexpr rt::vector min_empty = rt::vector( infinity,  infinity,  infinity);
    static constexpr rt::vector max_empty = rt::vector(-infinity, -infinity, -infinity);
};

struct min_max_coord {
    real min_x, max_x, min_y, max_y, min_z, max_z;

    void update(min_max_vectors& min_max) const {

        auto& [ min, max ] = min_max;

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

    void print() const {
        std::cout
            <<    "x: [" << min_x << "; " << max_x
            << "]; y: [" << min_y << "; " << max_y
            << "]; z: [" << min_z << "; " << max_z << "]";
    }
};

constexpr min_max_coord empty_set_min_max_coords = {

    .min_x =  infinity,
    .max_x = -infinity,

    .min_y =  infinity,
    .max_y = -infinity,

    .min_z =  infinity,
    .max_z = -infinity
};

constexpr min_max_coord build_min_max_coord(const min_max_vectors& min_max) {

    const auto& [ min, max ] = min_max;

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
min_max_vectors compute_bounding_vectors(const std::vector<const T*>& set) {
    
    min_max_vectors min_max;

    for (const T* p : set)
        p->get_min_max_coord().update(min_max);
    
    return min_max;
}