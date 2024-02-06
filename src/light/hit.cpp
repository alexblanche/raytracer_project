#include "headers/hit.hpp"
#include "../screen/headers/color.hpp"
#include "headers/vector.hpp"
#include "../auxiliary/headers/randomgen.hpp"
#include <cmath>

/** The hit class contains the information
 * of a ray hitting a surface: the ray in question,
 * the point of contact, the normal of the surface at
 * this point and the index of the object the surface belongs to.
*/

/* Constructors */

/* Main constructor */
hit::hit(const ray& g, const rt::vector& p, const rt::vector& n, const unsigned int i)
    : gen(g), point(p), normal(n), obj_index(i) {}


/* Default constructor */
hit::hit() {
    gen = ray();
    point = rt::vector();
    normal = rt::vector();
    obj_index = 0;
}

/* Accessors */

ray hit::get_ray() const {
    return gen;
}

rt::vector hit::get_point() const {
    return point;
}

rt::vector hit::get_normal() const {
    return normal;
}

unsigned int hit::get_obj_index() const {
    return obj_index;
}

/* Reflection */

/* Returns the reflected ray at the point of contact */
ray hit::reflect_ray() const {

    /* ray::direction and hit::normal are supposed to be unit vectors
       u is directed toward the surface, so the cos is computed with (-u),
       and so is the reflected ray: (2*cos*normal - (-u)) */
    const rt::vector u = gen.get_direction();
    const double cos = (-1) * (u | normal);

    return ray(point, (2*cos)*normal + u);
}

/* Returns a vector of n random reflected ray passing through the disk of given radius,
   with center hit::reflect_ray(), at given distance from the point of contact */

/* The point of origin is u.
   To generate a random vector in the disk, we first compute two vectors X, Y,
   both orthogonal and orthogonal to a.
   We define r = rand(0,1), theta = rand(0,2*pi),
   and the vector is: distance * central_dir + radius * sqrt(r) * ((cos theta) * X + (sin theta) * Y)

   The sqrt is applied so that:
   P[pt \in disk of surface x^2] = P[sqrt(r) <= x] = P[r <= x^2] = x^2,
   i.e. the points are evenly distributed across the unit disk.
*/

/* To obtain the two vectors X, Y:
   central_dir = (a,b,c). One of a,b,c is different from 0, let us say a.
   X = (-b/a, 1, 0) is a solution.
   We now look for a Y that satisfies ((a,b,c) | X) = 0 and (X | Y) = 0.
   Y = (ac, bc, -(a*a + b*b)) is a solution.
*/

std::vector<ray> hit::random_reflect(const unsigned int n, randomgen& rg,
    const double reflectivity, const double theta_max) const {

    const double twopi = 2 * 3.14159265358979323846;

    // n random doubles between 0 and 1, and n between 0 and 2*pi
    const std::vector<double> rands01 = random_double_array(rg, n, 1);
    const std::vector<double> rands0twopi = random_double_array(rg, n, twopi);

    // Central direction of the rays
    const rt::vector central_dir = (reflectivity * reflect_ray().get_direction() + (1 - reflectivity) * get_normal()).unit();
    const double a = central_dir.x;
    const double b = central_dir.y;
    const double c = central_dir.z;

    // Orthonormal base of the plane orthogonal to central_dir
    rt::vector X, Y;
    if (a != 0) {
        X = rt::vector(- b / a, 1, 0).unit();
        Y = rt::vector(a * c, b * c, - a*a - b*b).unit();
    } else if (b != 0) {
        // central_dir = (0,b,c)
        X = rt::vector(0, - c / b, 1).unit();
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
