#pragma once

#include "light/ray.hpp"
#include "auxiliary/min_max_coord.hpp"

/* Axis-aligned bounding box */

class aabb {
    private:
        rt::vector corner;
        rt::vector dims;

    public:

        aabb(const rt::vector& corner, const rt::vector& dims)
            : corner(corner), dims(dims) {}

        real measure_distance(const ray& r) const;

        bool is_hit_by(const ray& r) const;

        /* Minimum and maximum coordinates */
        min_max_coord get_min_max_coord() const {
            return build_min_max_coord(corner, corner + dims);
        }
};