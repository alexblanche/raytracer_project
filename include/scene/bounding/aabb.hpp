#pragma once

#include "light/ray.hpp"
#include "auxiliary/min_max_coord.hpp"

/* Axis-aligned bounding box */

class aabb {
    private:
        rt::vector corner;
        rt::vector dims;

    public:

        inline static unsigned int cpt = 0;

        aabb(const rt::vector& corner, const rt::vector& dims)
            :   corner(corner), dims(dims) {cpt++;}

        /* Only measures the distance from the outside of the aabb, otherwise returns 0.0_r */
        real measure_distance(const ray& r) const;

        inline bool is_hit_by(const ray& r) const {
            return measure_distance(r) < infinity;
        }

        /* Minimum and maximum coordinates */
        inline min_max_coord get_min_max_coord() const {
            return build_min_max_coord(corner, corner + dims);
        }

        /* Returns the corner */
        inline rt::vector get_position() const {
            return corner + dims;
        }
};