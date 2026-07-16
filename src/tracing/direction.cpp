#include "tracing/direction.hpp"

/* Returns the reflected ray at the point of contact */
/* Unused */
/*
ray directions::get_reflected_ray() {

    * ray::direction and hit::normal are supposed to be unit vectors
       u is directed toward the surface, so the cos is computed with (-u),
       and so is the reflected ray: (2*cos*normal - (-u)) *
    const rt::vector u = generator.get_direction();
    const real cos = (-1) * (u | normal);

    return ray(point, (2*cos)*normal + u);
}
*/

/* Returns the interpolated direction between the normal and the reflected direction */
/* inward = ((direction | normal) <= 0) */
rt::vector direction::central_reflected(const rt::vector& direction, const rt::vector& normal, const real smoothness, const orientation_type ray_orientation) {
    /*
    u = direction
    inward = (u | normal) <= 0;
    right_normal = inward ? normal : (-1)*normal;
    cos = -(u | normal)
    reflected_dir = (2*cos) * right_normal + u
    smoothness * reflected_dir + (1 - smoothness) * right_normal
        = (r * (2 * cos - 1) + 1) * right_normal + r * u
    */
    const real correcting_factor = (ray_orientation == orientation_type::Inward) ? 1.0_r : -1.0_r;
    const real cos = (-correcting_factor) * (direction | normal);
    //return (smoothness * (2.0_r * cos - 1.0_r) + 1.0_r) * right_normal + smoothness * u;
    return fma(direction, smoothness, ((smoothness * (2.0_r * cos - 1.0_r) + 1.0_r) * correcting_factor) * normal);
}


// Run-time version (compile-time in hpp)
rt::vector direction::random(const randomgen& rg, const rt::vector& central_dir, const real theta_max) {
    
    const rt::vector random_dir = direction::random<angle::Pi>(rg);
    return (fma(random_dir, sin(theta_max), central_dir)).unit();
}


/* Refraction */

/* Computes the Fresnel coefficient Kr */
real direction::get_fresnel(const rt::vector& direction, const rt::vector& normal,
    const real sin_theta_2_sq, const real refr_1, const real refr_2) {

    const real pdt = (direction | normal);
    const real cos_theta_1 = std::abs(pdt);
    const real cos_theta_2 = sqrt(1.0_r - sin_theta_2_sq);

    const real refr1costheta1 = refr_1 * cos_theta_1;
    const real refr1costheta2 = refr_1 * cos_theta_2;
    const real refr2costheta1 = refr_2 * cos_theta_1;
    const real refr2costheta2 = refr_2 * cos_theta_2;
    const real orth = (refr2costheta1 - refr1costheta2) / (refr2costheta1 + refr1costheta2);
    const real para = (refr2costheta2 - refr1costheta1) / (refr2costheta2 + refr1costheta1);
    
    return (para * para + orth * orth) / 2.0_r;
}

/* Compute Schlick's approximation of Fresnel coefficient Kr */
real direction::get_schlick(const rt::vector& direction, const rt::vector& normal, const real refr_1, const real refr_2) {

    const real pdt = (direction | normal);
    const real cos_theta_1 = std::abs(pdt);

    const real ratio = (refr_1 - refr_2) / (refr_1 + refr_2);
    const real r_zero = ratio * ratio;
    return r_zero + (1.0_r - r_zero) * pow(1.0_r - cos_theta_1, 5);
}