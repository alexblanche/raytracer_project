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

    inline rt::vector direction(const real sin_phi, const real theta) {

        /* cos(phi) is chosen to be positive */
        const real cos_phi = sqrt(1.0_r - sin_phi * sin_phi);

        return rt::vector(
            cos(theta) * cos_phi,
            sin_phi,
            sin(theta) * cos_phi
        );
    }

    /* Uniform sampling of a point on the surface of a unit sphere */
    inline rt::vector random_direction_sphere(const randomgen& rg) {

        /* sin(phi) is sampled uniformly on [-1, 1] */
        const real sin_phi = 2.0_r * rg.random_ratio() - 1.0_r;

        /* theta is sampled uniformly on [0, 2pi] */
        const real theta = rg.random_angle();

        return trig::direction(sin_phi, theta);
    }

    inline angles get_angles(const rt::vector& dir) {

        return {
            .theta = (is_not_zero(dir.x)) ?
                  atan(dir.z / dir.x) + (is_positive(dir.x) ? (3.0_r * PI / 2.0_r) : (PI / 2.0_r))
                : 0.0_r,

            /* Determining the pixel of the background texture to display */
            // dir is a unit vector, but due to floating-point imprecision, dir.y can be greater than 1
            .phi = (not abs_less_than_one(dir.y)) ?
                  (is_positive(dir.y) ? PI : 0.0_r)
                : asin(dir.y) + PI / 2.0_r
        };
    }
};