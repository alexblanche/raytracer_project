#pragma once

#include "math/geometry/vector.hpp"
#include "auxiliary/randomgen.hpp"
#include "auxiliary/utils.hpp"

namespace trig {

    /*
        theta = horizontal angle (along the y=0 plane, theta = 0: positive x axis)
        phi   = vertical angle (phi = 0: (0,1,0), phi = pi/2: y=0 plane, phi = pi: (0,-1,0))
    */
    struct angles {
        const real theta;
        const real phi;
    };

    inline rt::vector direction(const real cos_phi, const real theta) {

        /* sin(phi) is chosen to be positive */
        const real sin_phi = sqrt(1.0_r - cos_phi * cos_phi);

        return rt::vector(
            cos(theta) * sin_phi,
            cos_phi,
            sin(theta) * sin_phi
        );
    }

    /* Uniform sampling of a point on the surface of a unit sphere */
    inline rt::vector random_direction_sphere(const randomgen& rg) {

        /* cos(phi) is sampled uniformly on [-1, 1] */
        const real cos_phi = 2.0_r * rg.random_ratio() - 1.0_r;

        /* theta is sampled uniformly on [0, 2pi] */
        const real theta = rg.random_angle();

        return trig::direction(cos_phi, theta);
    }

    inline angles get_angles(const rt::vector& dir) {

        const auto& [ x, y, z ] = dir;

        return {
            .theta = (is_not_zero(x)) ?
                  atan(z / x) + (is_positive(x) ? (3.0_r * PI / 2.0_r) : (PI / 2.0_r))
                : 0.0_r,

            // dir is a unit vector, but due to floating-point imprecision, dir.y can be greater than 1
            .phi = (not abs_less_than_one(y)) ?
                  (is_positive(y) ? 0.0_r : PI)
                : acos(y)
        };
    }
};