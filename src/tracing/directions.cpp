#include "tracing/directions.hpp"

#include "auxiliary/utils.hpp"

/* Returns the reflected ray at the point of contact */
/* Unused */
/*
ray get_reflected_ray() {

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
rt::vector get_central_reflected_direction(const hit& h, const rt::vector& normal, const real smoothness, const orientation_type ray_orientation) {
    const rt::vector& u = h.get_generator_ray()->direction;
    /*
    inward = (u | normal) <= 0;
    right_normal = inward ? normal : (-1)*normal;
    cos = -(u | normal)
    reflected_dir = (2*cos) * right_normal + u
    smoothness * reflected_dir + (1 - smoothness) * right_normal
        = (r * (2 * cos - 1) + 1) * right_normal + r * u
    */
    const real correcting_factor = (ray_orientation == orientation_type::Inward) ? 1.0_r : -1.0_r;
    const real cos = (-correcting_factor) * (u | normal);
    //return (smoothness * (2.0_r * cos - 1.0_r) + 1.0_r) * right_normal + smoothness * u;
    return fma(u, smoothness, ((smoothness * (2.0_r * cos - 1.0_r) + 1.0_r) * correcting_factor) * normal);
}


// Run-time version (compile-time in hpp)
rt::vector random_direction(const randomgen& rg, const rt::vector& central_dir, const real theta_max) {

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

    const real cos_theta_max = cos(theta_max);
    const real cos_theta = 1.0_r - p * (1.0_r - cos_theta_max);
    const real sin_theta = sqrt(1.0_r - cos_theta * cos_theta);
    
    return
        matprod(
            X,           cos(phi) * sin_theta,
            Y,           sin(phi) * sin_theta,
            central_dir, cos_theta
        );
}


/* Refraction */

/* Returns sin(theta_2), where theta_2 is the refracted angle
   Is precomputed to determine whether the ray is refracted or internally reflected */
rt::vector get_sin_refracted(const hit& h, const rt::vector& normal,
    const real current_refr_i, const real surface_refr_i,
    real& sin_theta_2_sq) {

    /* See get_refracted_direction below */
    const rt::vector& dir = h.get_generator_ray()->direction;
    /* It should be (current_refr_i / surface_refr_i) * ((((-1)*(dir | right_normal)) * right_normal) + dir)
       where right_normal = inward ? normal : (-1) * normal,
       but the next line is equivalent */
    //const rt::vector vx = (current_refr_i / surface_refr_i) * ((((-1.0_r) * (dir | normal)) * normal) + dir);
    const rt::vector vx = (current_refr_i / surface_refr_i) * fma(normal, (-1.0_r) * (dir | normal), dir);
    sin_theta_2_sq = vx.normsq();
    return vx;
}

/* Returns the refracted direction */
rt::vector get_refracted_direction(const rt::vector& normal, const rt::vector& vx, const real sin_theta_2_sq,
    const orientation_type ray_orientation) {
    
    /* Factor to apply to normal to obtain the normal outward the surface of contact,
       so that (dir | a * normal) <= 0 */
    // const real a = inward ? 1 : (-1);

    /* Snell-Descartes law */
    /* If the angle between (-dir) and a*normal is theta_1,
       the angle between the refracted direction v and (-a*normal) is theta_2,
       then current_refr_i * sin(theta_1) = surface_refr_i * sin(theta_2)

       sin(theta_1) = |(-dir | (a * normal)) * (a*normal) - (-dir)|
       So sin(theta_2) = (current_refr_i / surface_refr_i) * |(dir | (a * normal)) * (a*normal) - dir|

       The refracted direction v can be decomposed into v = vx + vy,
       where vx is coplanar to dir and normal, orthogonal to normal:
       vx = (current_refr_i / surface_refr_i) * ((dir | a*normal) * (a*normal) - dir)
       and vy = sqrt(1 - vx.normsq()) * (-a)*normal,
       so that v is a unit vector
    */

    //return vx + sqrt(1.0_r - sin_theta_2_sq) * (inward ? (-1.0_r) * normal : normal);
    return fma(
        normal,
        (ray_orientation == orientation_type::Inward ? (-1.0_r) : 1.0_r) * sqrt(1.0_r - sin_theta_2_sq),
        vx
    );
}

/* Returns a random unit direction in the cone whose center is the refracted direction, within solid angle refraction_scattering * pi */
rt::vector get_random_refracted_direction(const randomgen& rg, const real refraction_scattering,
    const rt::vector& normal,
    const rt::vector& vx, const real sin_theta_2_sq,
    const orientation_type ray_orientation) {

    const rt::vector refr_dir = get_refracted_direction(normal, vx, sin_theta_2_sq, ray_orientation);
    return random_direction(rg, refr_dir, refraction_scattering * (PI / 2.0));
}

/* Computes the Fresnel coefficient Kr */
real get_fresnel(const hit& h, const rt::vector& normal,
    const real sin_theta_2_sq, const real refr_1, const real refr_2) {

    const real pdt = (h.get_generator_ray()->direction | normal);
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
real get_schlick(const hit& h, const rt::vector& normal, const real refr_1, const real refr_2) {
    const real pdt = (h.get_generator_ray()->direction | normal);
    const real cos_theta_1 = std::abs(pdt);

    const real ratio = (refr_1 - refr_2) / (refr_1 + refr_2);
    const real r_zero = ratio * ratio;
    return r_zero + (1.0_r - r_zero) * pow(1.0_r - cos_theta_1, 5);
}