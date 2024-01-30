#include "headers/hit.hpp"
#include "../screen/headers/color.hpp"
#include "headers/vector.hpp"

/** The hit class contains the information
 * of a ray hitting a surface: the ray in question,
 * the point of contact, the normal of the surface at
 * this point and the color of the surface.
*/

/* Constructors */

/* Main constructor */
hit::hit(const ray& g, const rt::vector& p, const rt::vector& n, const rt::color& c)
    : gen(g), point(p), normal(n), col(c) {}


/* Default constructor */
hit::hit() {
    gen = ray();
    point = rt::vector();
    normal = rt::vector();
    col = rt::color::BLACK;
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

rt::color hit::get_color() const {
    return col;
}




