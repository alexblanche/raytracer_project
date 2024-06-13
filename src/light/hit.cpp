#include "light/hit.hpp"
#include "light/vector.hpp"
#include "scene/objects/object.hpp"
#include "auxiliary/randomgen.hpp"
#include <cmath>

#define TWOPI 6.2831853071795862f

/** The hit class contains the information
 * of a ray hitting a surface: the ray in question,
 * the point of contact, the normal of the surface at
 * this point and the index of the object the surface belongs to.
*/

/* Constructors */

/* Main constructor */
hit::hit(ray*& generator, const rt::vector& point, const rt::vector& normal, const object*& hit_object)
    : generator(generator), point(point), normal(normal), hit_object(hit_object) {}

hit::hit() {}


/* Reflection */

/* Returns the reflected ray at the point of contact */
/* Unused */
/*
ray hit::get_reflected_ray() const {

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
rt::vector hit::get_central_reflected_direction(const real& reflectivity, const bool inward) const {
    const rt::vector u = generator->get_direction();
    /*
    inward = (u | normal) <= 0;
    right_normal = inward ? normal : (-1)*normal;
    cos = -(u | normal)
    reflected_dir = (2*cos) * right_normal + u
    reflectivity * reflected_dir + (1 - reflectivity) * right_normal
        = (r * (2 * cos - 1) + 1) * right_normal + r * u
    */
    const rt::vector right_normal = inward ? normal : (-1) * normal;
    const real cos = (-1.0f) * (u | right_normal);
    return (reflectivity * (2.0f * cos - 1.0f) + 1.0f) * right_normal + reflectivity * u;
}



/* Returns a vector of n random reflected ray in the cone of center hit::reflect_ray(),
   within solid angle theta_max */

/* To obtain two vectors X, Y orthogonal to central_dir:
   central_dir = (a,b,c). One of a,b,c is different from 0, let us say a.
   X = (-b/a, 1, 0) is a solution.
   We now look for a Y that satisfies ((a,b,c) | X) = 0 and (X | Y) = 0.
   Y = (ac, bc, -(a*a + b*b)) is a solution.
*/

std::vector<ray> hit::random_reflect(const size_t n, randomgen& rg,
    const rt::vector& central_dir, const real& theta_max) const {

    // n random reals between 0 and 1, and n between 0 and 2*pi
    const std::vector<real> rands01 = rg.random_real_array(n, 1.0f);
    const std::vector<real> rands0twopi = rg.random_real_array(n, TWOPI);

    // Central direction of the rays
    const real a = central_dir.x;
    const real b = central_dir.y;
    const real c = central_dir.z;

    // Orthonormal base of the plane orthogonal to central_dir
    rt::vector X, Y;
    if (a != 0.0f) {
        X = rt::vector(-b, a, 0.0f).unit();
        Y = rt::vector(a * c, b * c, -a*a -b*b).unit();
    } else if (b != 0.0f) {
        // central_dir = (0,b,c)
        X = rt::vector(0.0f, -c, b).unit();
        Y = rt::vector(1.0f, 0.0f, 0.0f);
    } else {
        // central_dir = (0,0,1)
        X = rt::vector(1.0f, 0.0f, 0.0f);
        Y = rt::vector(0.0f, 1.0f, 0.0f);
    }

    const real cos_theta_max = cos(theta_max);

    // vector of random rays in the cone of angle theta_max to central_dir
    std::vector<ray> rays;
    rays.reserve(n);
    for (size_t i = 0; i < n; i++) {
        const real p = rands01[i];
        const real phi = rands0twopi[i];

        /*
        theta = acos(1 - p(1 - cos(theta_max)))
        x = cos(phi) sin(theta) = cos(phi) * sqrt(1 - (1 - p(1-cos(theta_max))^2))
        y = sin(phi) sin(theta) = sin(phi) * sqrt(1 - (1 - p(1-cos(theta_max))^2))
        z = cos(theta)          = 1 - p(1-cos(theta_max))
        */
        const real cos_theta = 1.0f - p * (1.0f - cos_theta_max);
        rays.emplace_back(point,
              (cos(phi) * sqrt(1.0f - cos_theta * cos_theta)) * X
            + (sin(phi) * sqrt(1.0f - cos_theta * cos_theta)) * Y
            + cos_theta * central_dir);
    }

    return rays;
}

/* Returns a random unit direction in the cone of center central_dir, within solid angle theta_max */
rt::vector hit::random_direction(randomgen& rg, const rt::vector& central_dir, const real& theta_max) const {

    // n random reals between 0 and 1, and n between 0 and 2*pi
    const real p = rg.random_real(1.0f);
    const real phi = rg.random_real(TWOPI);

    // Central direction of the rays
    const real& a = central_dir.x;
    const real& b = central_dir.y;
    const real& c = central_dir.z;

    // Orthonormal base of the plane orthogonal to central_dir
    rt::vector X, Y;
    if (a != 0.0f) {
        X = rt::vector(- b, a, 0.0f).unit();
        Y = rt::vector(a * c, b * c, (-1.0f) * (a*a + b*b)).unit();
    } else if (b != 0.0f) {
        // central_dir = (0,b,c)
        X = rt::vector(0.0f, - c, b).unit();
        Y = rt::vector(1.0f, 0.0f, 0.0f);
    } else {
        // central_dir = (0,0,1)
        X = rt::vector(1, 0.0f, 0.0f);
        Y = rt::vector(0.0f, 1, 0.0f);
    }

    const real cos_theta_max = cos(theta_max);

    // random ray in the cone of angle theta_max to central_dir
    /*
    theta = acos(1 - p(1 - cos(theta_max)))
    x = cos(phi) sin(theta) = cos(phi) * sqrt(1 - (1 - p(1-cos(theta_max))^2))
    y = sin(phi) sin(theta) = sin(phi) * sqrt(1 - (1 - p(1-cos(theta_max))^2))
    z = cos(theta)          = 1 - p(1-cos(theta_max))
    */
    const real cos_theta = 1.0f - p * (1.0f - cos_theta_max);
    const real sin_theta = sqrt(1.0f - cos_theta * cos_theta);
    
    return (
          (cos(phi) * sin_theta) * X
        + (sin(phi) * sin_theta) * Y
        + cos_theta * central_dir);
}


/* Refraction */

/* Returns sin(theta_2), where theta_2 is the refracted angle
   Is precomputed to determine whether the ray is refracted or internally reflected */
rt::vector hit::get_sin_refracted(const real& current_refr_i, const real& surface_refr_i,
    real& sin_theta_2_sq) const {

    /* See get_refracted_direction below */
    const rt::vector& dir = generator->get_direction();
    /* It should be (current_refr_i / surface_refr_i) * ((((-1)*(dir | right_normal)) * right_normal) + dir)
       where right_normal = inward ? normal : (-1) * normal,
       but the next line is equivalent */
    const rt::vector vx = (current_refr_i / surface_refr_i) * ((((-1.0f) * (dir | normal)) * normal) + dir);
    sin_theta_2_sq = vx.normsq();
    return vx;
}

/* Returns the refracted direction */
rt::vector hit::get_refracted_direction(const rt::vector& vx, const real& sin_theta_2_sq, const bool inward) const {
    
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

    return vx + sqrt(1.0f - sin_theta_2_sq) * (inward ? (-1.0f) * normal : normal);
}

/* Returns a random unit direction in the cone whose center is the refracted direction, within solid angle refraction_scattering * pi */
rt::vector hit::get_random_refracted_direction(randomgen& rg, const real& refraction_scattering,
    const rt::vector& vx, const real& sin_theta_2_sq, const bool inward) const {

    const rt::vector refr_dir = get_refracted_direction(vx, sin_theta_2_sq, inward);
    return random_direction(rg, refr_dir, refraction_scattering * 1.57079632679f);
}

/* Computes the Fresnel coefficient Kr */
real hit::get_fresnel(const real& sin_theta_2_sq, const real& refr_1, const real& refr_2) const {

    const real pdt = (-1.0f) * (generator->get_direction() | normal);
    const real cos_theta_1 = abs(pdt);
    const real cos_theta_2 = sqrt(1.0f - sin_theta_2_sq);

    const real orth = (refr_2 * cos_theta_1 - refr_1 * cos_theta_2) / (refr_2 * cos_theta_1 + refr_1 * cos_theta_2);
    const real para = (refr_2 * cos_theta_2 - refr_1 * cos_theta_1) / (refr_2 * cos_theta_2 + refr_1 * cos_theta_1);
    
    return (para * para + orth * orth) / 2.0f;
}

/* Compute Schlick's approximation of Fresnel coefficient Kr */
real hit::get_schlick(const real& refr_1, const real& refr_2) const {
    const real pdt = (-1.0f) * (generator->get_direction() | normal);
    const real cos_theta_1 = abs(pdt);

    const real r_zero = pow((refr_1 - refr_2) / (refr_1 + refr_2), 2);

    return r_zero + (1.0f - r_zero) * pow(1.0f - cos_theta_1, 5);
}