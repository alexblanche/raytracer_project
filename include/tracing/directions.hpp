#pragma once

#include "light/hit.hpp"
#include "auxiliary/randomgen.hpp"

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
rt::vector random_direction(randomgen& rg, const rt::vector& central_dir,
    const real theta_max);

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