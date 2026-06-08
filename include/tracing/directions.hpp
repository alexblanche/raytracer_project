#pragma once

#include "light/hit.hpp"
#include "auxiliary/randomgen.hpp"
#include "auxiliary/utils.hpp"

/* Returns the reflected ray at the point of contact */
// ray get_reflected_ray() const;

template<orientation_type ray_orientation>
/* Returns the interpolated direction between the normal and the reflected direction */
/* inward = ((direction | normal) <= 0) */
rt::vector get_central_reflected_direction(const hit& h, const rt::vector& normal, const real smoothness) {
    constexpr real correcting_factor = (ray_orientation == orientation_type::Inward) ? 1.0_r : -1.0_r;
    constexpr real two_corr_f = -2.0_r * correcting_factor;

    const rt::vector& u = h.get_generator_ray()->get_direction();
    const real two_cos = two_corr_f * (u | normal);
    //return (smoothness * (2.0_r * cos - 1.0_r) + 1.0_r) * right_normal + smoothness * u;
    return fma(u, smoothness, ((smoothness * (two_cos - 1.0_r) + 1.0_r) * correcting_factor) * normal);
}

/* Returns the interpolated direction between the normal and the reflected direction */
/* inward = ((direction | normal) <= 0) */
rt::vector get_central_reflected_direction(const hit& h, const rt::vector& normal, real reflectivity, orientation_type ray_orientation);

enum class angle {
    Pi
};

/* Returns a random unit direction in the cone of center central_dir, within solid angle theta_max */
// Constexpr theta_max
/* Returns a random unit direction in the cone of center central_dir, within solid angle theta_max */
template <angle theta_max>
rt::vector random_direction(const randomgen& rg, const rt::vector& central_dir) {

    constexpr real cos_theta_max =
          (theta_max == angle::Pi) ? -1.0_r
        : /* placeholder */           0.0_r;

    constexpr real one_m_costhetamax = 1.0_r - cos_theta_max;

    // random ray in the cone of angle theta_max to central_dir
    /*
    theta = acos(1 - p(1 - cos(theta_max)))
    x = cos(phi) sin(theta) = cos(phi) * sqrt(1 - (1 - p(1-cos(theta_max))^2))
    y = sin(phi) sin(theta) = sin(phi) * sqrt(1 - (1 - p(1-cos(theta_max))^2))
    z = cos(theta)          = 1 - p(1-cos(theta_max))
    */
    if constexpr (one_m_costhetamax != 0.0_r) {

        const real p   = rg.random_ratio();
        const real phi = rg.random_angle();

        // Central direction of the rays
        const auto [ a, b, c ] = central_dir;

        // Orthonormal base of the plane orthogonal to central_dir
        rt::vector X, Y;
        if (is_not_zero(a)) {
            const real nX = a * a + b * b;
            const real sqrtnX = sqrt(nX);
            X = rt::vector(- b / sqrtnX, a / sqrtnX, 0.0_r);
            Y = rt::vector(a * c, b * c, -nX).unit();
        }
        else if (is_not_zero(b)) {
            // central_dir = (0,b,c)
            X = rt::vector(0.0_r, - c, b).unit();
            Y = rt::vector(1, 0, 0);
        }
        else {
            // central_dir = (0,0,1)
            X = rt::vector(1, 0, 0);
            Y = rt::vector(0, 1, 0);
        }

        const real cos_theta = 1.0_r - p * one_m_costhetamax;
        const real sin_theta = sqrt(1.0_r - cos_theta * cos_theta);
        
        return
            matprod(
                X,           cos(phi) * sin_theta,
                Y,           sin(phi) * sin_theta,
                central_dir, cos_theta
            );
    }
    else {
        // constexpr real cos_theta = 1.0;
        // constexpr real sin_theta = 0.0;
        
        return central_dir;
    }
}
// Run-time theta_max
rt::vector random_direction(const randomgen& rg, const rt::vector& central_dir, real theta_max);

/* Refraction */

/* Returns sin(theta_2) squared, where theta_2 is the refracted angle
    Is precomputed to determine whether the ray is refracted or internally reflected */
rt::vector get_sin_refracted(const hit& h, const rt::vector& normal,
    real current_refr_i, real surface_refr_i,
    real& sin_theta_2_sq);

/* Returns the refracted direction */
rt::vector get_refracted_direction(const rt::vector& normal, const rt::vector& vx, real sin_theta_2_sq, orientation_type ray_orientation);

/* Returns a random unit direction in the cone whose center is the refracted direction, within solid angle refraction_scattering * pi */
rt::vector get_random_refracted_direction(const randomgen& rg, real refraction_scattering,
    const rt::vector& normal,
    const rt::vector& vx, real sin_theta_2_sq, orientation_type ray_orientation);

/* Computes the Fresnel coefficient Kr */
real get_fresnel(const hit& h, const rt::vector& normal, real sin_theta_2_sq, real refr_1, real refr_2);

/* Compute Schlick's approximation of Fresnel coefficient Kr */
real get_schlick(const hit& h, const rt::vector& normal, real refr_1, real refr_2);