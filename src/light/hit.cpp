#include "light/hit.hpp"
#include "light/vector.hpp"
#include "screen/color.hpp"
#include "auxiliary/randomgen.hpp"
#include <cmath>

/** The hit class contains the information
 * of a ray hitting a surface: the ray in question,
 * the point of contact, the normal of the surface at
 * this point and the index of the object the surface belongs to.
*/

/* Constructors */

/* Main constructor */
hit::hit(const ray& g, const rt::vector& p, const rt::vector& n, const unsigned int i)
    : gen(g), point(p), normal(n), obj_index(i), is_hit_bool(true) {}


/* Default constructor */
hit::hit() : is_hit_bool(false) {}


/* Reflection */

/* Returns the reflected ray at the point of contact */
ray hit::get_reflected_ray() const {

    /* ray::direction and hit::normal are supposed to be unit vectors
       u is directed toward the surface, so the cos is computed with (-u),
       and so is the reflected ray: (2*cos*normal - (-u)) */
    const rt::vector& u = gen.get_direction();
    const double cos = (-1) * (u | normal);

    return ray(point, (2*cos)*normal + u);
}

/* Returns the interpolated direction between the normal and the reflected direction */
rt::vector hit::get_central_direction(const double reflectivity) const {
    const rt::vector& u = gen.get_direction();
    const double cos = (-1) * (u | normal);
    return (reflectivity * ((2*cos - 1)*normal + u) + normal).unit();
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

/* Returns the direction of a random reflected ray in the cone of center hit::reflect_ray(),
   within solid angle theta_max */
rt::vector hit::random_reflect_single(randomgen& rg, const rt::vector& central_dir, const double theta_max) const {

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