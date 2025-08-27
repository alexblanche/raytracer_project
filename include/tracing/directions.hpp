#pragma once

#include "light/hit.hpp"
#include "auxiliary/randomgen.hpp"

constexpr real PI = 3.14159265358979323846;

/* Returns the reflected ray at the point of contact */
// ray get_reflected_ray() const;

/* Returns the interpolated direction between the normal and the reflected direction */
/* inward = ((direction | normal) <= 0) */
rt::vector get_central_reflected_direction(const hit& h, const rt::vector& normal, const real reflectivity, const bool inward);

/* Returns a vector of n random reflected ray in the cone of center hit::reflect_ray(),
    within solid angle theta_max */
// std::vector<ray> random_reflect(const size_t n, randomgen& rg,
//     const rt::vector& central_dir, const real theta_max) const;

/* Returns a random unit direction in the cone of center central_dir, within solid angle theta_max */
// Constexpr theta_max
/* Returns a random unit direction in the cone of center central_dir, within solid angle theta_max */
template <real theta_max>
rt::vector random_direction(randomgen& rg, const rt::vector& central_dir) {

    constexpr real cos_theta_max = std::cos(theta_max);
    constexpr real one_m_costhetamax = 1.0f - cos_theta_max;

    // random ray in the cone of angle theta_max to central_dir
    /*
    theta = acos(1 - p(1 - cos(theta_max)))
    x = cos(phi) sin(theta) = cos(phi) * sqrt(1 - (1 - p(1-cos(theta_max))^2))
    y = sin(phi) sin(theta) = sin(phi) * sqrt(1 - (1 - p(1-cos(theta_max))^2))
    z = cos(theta)          = 1 - p(1-cos(theta_max))
    */
    if constexpr (one_m_costhetamax != 0) {

        const real p = rg.random_ratio();
        const real phi = rg.random_angle();

        // Central direction of the rays
        const real a = central_dir.x;
        const real b = central_dir.y;
        const real c = central_dir.z;

        // Orthonormal base of the plane orthogonal to central_dir
        rt::vector X, Y;
        if (a != 0.0f) {
            const real nX = a * a + b * b;
            X = rt::vector(- b, a, 0.0f) / sqrt(nX);
            Y = rt::vector(a * c, b * c, -nX).unit();
        } else if (b != 0.0f) {
            // central_dir = (0,b,c)
            X = rt::vector(0.0f, - c, b).unit();
            Y = rt::vector(1.0f, 0.0f, 0.0f);
        } else {
            // central_dir = (0,0,1)
            X = rt::vector(1.0f, 0.0f, 0.0f);
            Y = rt::vector(0.0f, 1.0f, 0.0f);
        }

        const real cos_theta = 1.0f - p * one_m_costhetamax; //(1.0f - cos_theta_max);
        const real sin_theta = sqrt(1.0f - cos_theta * cos_theta);
        
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
rt::vector random_direction(randomgen& rg, const rt::vector& central_dir, const real theta_max);

/* Refraction */

/* Returns sin(theta_2) squared, where theta_2 is the refracted angle
    Is precomputed to determine whether the ray is refracted or internally reflected */
rt::vector get_sin_refracted(const hit& h, const rt::vector& normal,
    const real current_refr_i, const real surface_refr_i,
    real& sin_theta_2_sq);

/* Returns the refracted direction */
rt::vector get_refracted_direction(const rt::vector& normal, const rt::vector& vx, const real sin_theta_2_sq, const bool inward);

/* Returns a random unit direction in the cone whose center is the refracted direction, within solid angle refraction_scattering * pi */
rt::vector get_random_refracted_direction(randomgen& rg, const real refraction_scattering,
    const rt::vector& normal,
    const rt::vector& vx, const real sin_theta_2_sq, const bool inward);

/* Computes the Fresnel coefficient Kr */
real get_fresnel(const hit& h, const rt::vector& normal, const real sin_theta_2_sq, const real refr_1, const real refr_2);

/* Compute Schlick's approximation of Fresnel coefficient Kr */
real get_schlick(const hit& h, const rt::vector& normal, const real refr_1, const real refr_2);