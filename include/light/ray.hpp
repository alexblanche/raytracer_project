#pragma once

#include "light/vector.hpp"

struct ray {
    
    public:
        rt::vector origin;

        rt::vector direction;

        /* Pre-computed values */

        // Inverse of the direction
        rt::vector inv_dir;

        // Absolute values of each component
        // rt::vector abs_inv_dir;

        ray() = delete;
        
        ray(const rt::vector& origin, const rt::vector& dir)
            :
            origin(origin),
            direction(dir),
            inv_dir(
                1.0_r / direction.x,
                1.0_r / direction.y,
                1.0_r / direction.z
            )
            /* , abs_inv_dir(rt::abs(inv_dir)) */ {}

        ray& operator=(ray&&)      = default;
        ray(ray&&)                 = default;
        ray(const ray&)            = default;
        ray& operator=(const ray&) = delete;

        inline rt::vector extend(const real t) const {
            return fma(direction, t, origin);
        }
};

