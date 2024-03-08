#include "light/hit.hpp"
#include "light/vector.hpp"
#include "scene/objects/object.hpp"
#include "auxiliary/randomgen.hpp"
#include <cmath>

/** The hit class contains the information
 * of a ray hitting a surface: the ray in question,
 * the point of contact, the normal of the surface at
 * this point and the index of the object the surface belongs to.
*/

/* Constructors */

/* Main constructor */
hit::hit(ray*& generator, const rt::vector& point, const rt::vector& normal, const object*& hit_object)
    : generator(generator), point(point), normal(normal), hit_object(hit_object), is_hit_bool(true) {}


/* Default constructor */
hit::hit() : is_hit_bool(false) {}


/* Reflection */

/* Returns the reflected ray at the point of contact */
/* Unused */
/*
ray hit::get_reflected_ray() const {

    * ray::direction and hit::normal are supposed to be unit vectors
       u is directed toward the surface, so the cos is computed with (-u),
       and so is the reflected ray: (2*cos*normal - (-u)) *
    const rt::vector u = generator.get_direction();
    const double cos = (-1) * (u | normal);

    return ray(point, (2*cos)*normal + u);
}
*/

/* Returns the interpolated direction between the normal and the reflected direction */
/* inward = ((direction | normal) <= 0) */
rt::vector hit::get_central_reflected_direction(const double reflectivity, const bool inward) const {
    const rt::vector u = generator->get_direction();
    /* Optimization: the reflected direction is correct even if the ray is outward */
    const double cos = (-1) * (u | normal);
    return (reflectivity * ((2*cos - 1)*normal + u) + (inward ? 1 : -1) * normal).unit();
    // = (reflectivity * reflected_dir + (1 - reflectivity) * normal).unit()
}



/* Returns a vector of n random reflected ray in the cone of center hit::reflect_ray(),
   within solid angle theta_max */

/* To obtain two vectors X, Y orthogonal to central_dir:
   central_dir = (a,b,c). One of a,b,c is different from 0, let us say a.
   X = (-b/a, 1, 0) is a solution.
   We now look for a Y that satisfies ((a,b,c) | X) = 0 and (X | Y) = 0.
   Y = (ac, bc, -(a*a + b*b)) is a solution.
*/

std::vector<ray> hit::random_reflect(const unsigned int n, randomgen& rg,
    const rt::vector& central_dir, const double theta_max) const {

    const double twopi = 2 * 3.14159265358979323846;

    // n random doubles between 0 and 1, and n between 0 and 2*pi
    const std::vector<double> rands01 = random_double_array(rg, n, 1);
    const std::vector<double> rands0twopi = random_double_array(rg, n, twopi);

    // Central direction of the rays
    const double a = central_dir.x;
    const double b = central_dir.y;
    const double c = central_dir.z;

    // Orthonormal base of the plane orthogonal to central_dir
    rt::vector X, Y;
    if (a != 0) {
        X = rt::vector(-b, a, 0).unit();
        Y = rt::vector(a * c, b * c, -a*a -b*b).unit();
    } else if (b != 0) {
        // central_dir = (0,b,c)
        X = rt::vector(0, -c, b).unit();
        Y = rt::vector(1, 0, 0);
    } else {
        // central_dir = (0,0,1)
        X = rt::vector(1, 0, 0);
        Y = rt::vector(0, 1, 0);
    }

    const double cos_theta_max = cos(theta_max);

    // vector of random rays in the cone of angle theta_max to central_dir
    std::vector<ray> rays(n);
    for (unsigned int i = 0; i < n; i++) {
        const double p = rands01.at(i);
        const double phi = rands0twopi.at(i);

        /*
        theta = acos(1 - p(1 - cos(theta_max)))
        x = cos(phi) sin(theta) = cos(phi) * sqrt(1 - (1 - p(1-cos(theta_max))^2))
        y = sin(phi) sin(theta) = sin(phi) * sqrt(1 - (1 - p(1-cos(theta_max))^2))
        z = cos(theta)          = 1 - p(1-cos(theta_max))
        */
        const double cos_theta = 1 - p * (1-cos_theta_max);
        rays.at(i) = ray(point,
              (cos(phi) * sqrt(1 - cos_theta * cos_theta)) * X
            + (sin(phi) * sqrt(1 - cos_theta * cos_theta)) * Y
            + cos_theta * central_dir);
    }

    return rays;
}

/* Returns a random direction in the cone of center central_dir, within solid angle theta_max */
rt::vector hit::random_direction(randomgen& rg, const rt::vector& central_dir, const double theta_max) const {

    const double twopi = 2 * 3.14159265358979323846;

    // n random doubles between 0 and 1, and n between 0 and 2*pi
    const double p = random_double(rg, 1);
    const double phi = random_double(rg, twopi);

    // Central direction of the rays
    const double a = central_dir.x;
    const double b = central_dir.y;
    const double c = central_dir.z;

    // Orthonormal base of the plane orthogonal to central_dir
    rt::vector X, Y;
    if (a != 0) {
        X = rt::vector(- b, a, 0).unit();
        Y = rt::vector(a * c, b * c, - a*a - b*b).unit();
    } else if (b != 0) {
        // central_dir = (0,b,c)
        X = rt::vector(0, - c, b).unit();
        Y = rt::vector(1, 0, 0);
    } else {
        // central_dir = (0,0,1)
        X = rt::vector(1, 0, 0);
        Y = rt::vector(0, 1, 0);
    }

    const double cos_theta_max = cos(theta_max);

    // random ray in the cone of angle theta_max to central_dir
    /*
    theta = acos(1 - p(1 - cos(theta_max)))
    x = cos(phi) sin(theta) = cos(phi) * sqrt(1 - (1 - p(1-cos(theta_max))^2))
    y = sin(phi) sin(theta) = sin(phi) * sqrt(1 - (1 - p(1-cos(theta_max))^2))
    z = cos(theta)          = 1 - p(1-cos(theta_max))
    */
    const double cos_theta = 1 - p * (1-cos_theta_max);

    return (
          (cos(phi) * sqrt(1 - cos_theta * cos_theta)) * X
        + (sin(phi) * sqrt(1 - cos_theta * cos_theta)) * Y
        + cos_theta * central_dir);
}


/* Refraction */

/* Returns the refracted direction */
rt::vector hit::get_refracted_direction(const double current_refr_i, const double surface_refr_i) const {
    const rt::vector dir = generator->get_direction();
    /* Factor to apply to normal to obtain the normal outward the surface of contact,
       so that (dir | a * normal) <= 0 */
    const double a = ((dir | normal) <= 0) ? 1 : (-1);

    /* Snell-Descartes law */
    /* If the angle between (-dir) and a*normal is theta_1,
       the angle between the refracted direction v and (-a*normal) is theta_2,
       then current_refr_i * sin(theta_1) = surface_refr_i * sin(theta_2)

       sin(theta_1) = |(dir | (a * normal)) * (a*normal) - dir|
       So sin(theta_2) = (current_refr_i / surface_refr_i) * |(dir | (a * normal)) * (a*normal) - dir|

       The refracted direction v can be decomposed into v = vx + vy,
       where vx is coplanar to dir and normal, orthogonal to normal:
       vx = (current_refr_i / surface_refr_i) * ((dir | a*normal) * (a*normal) - dir)
       and vy = sqrt(1 - vx.normsq()) * (-a)*normal,
       so that v is a unit vector
    */

    /* Notice that a*a = 1 */
    const rt::vector vx = (current_refr_i / surface_refr_i) * ((dir | normal) * normal - dir);
    return vx + sqrt(1 - vx.normsq()) * ((-a) * normal);
}

/* Returns a random direction in the cone whose center is the refracted direction, within solid angle refraction_scattering * pi */
rt::vector hit::get_random_refracted_direction(randomgen& rg,
    const double current_refr_i, const double surface_refr_i, const double refraction_scattering) const {

    rt::vector refr_dir = get_refracted_direction(current_refr_i, surface_refr_i);
    return random_direction(rg, refr_dir, refraction_scattering * 3.14159265358979323846);
}