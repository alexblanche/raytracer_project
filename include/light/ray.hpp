#pragma once

#include "light/vector.hpp"

struct ray {
    
    public:
        rt::vector origin;
        rt::vector direction;

        /* Precomputed: inverse of the direction's components */
        rt::vector inv_dir;
        
        ray(const rt::vector& origin, const rt::vector& dir) :
            origin(origin), direction(dir),
            inv_dir(
                1.0_r / direction.x,
                1.0_r / direction.y,
                1.0_r / direction.z
            ) {}
        
        ray(ray&&)                 noexcept = default;
        ray(const ray&)            noexcept = default;
        ray& operator=(ray&&)      noexcept = default;
        ray& operator=(const ray&) = delete;

        [[nodiscard]] inline rt::vector extend(const real t) const {
            return fma(direction, t, origin);
        }
};

