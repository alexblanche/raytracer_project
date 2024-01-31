#include "headers/ray.hpp"
#include "../screen/headers/color.hpp"

using namespace std;

/** 
 * The ray class contains the information of a ray of light:
 * the point of origin, the direction vector and the color.
*/

/* Constructors */

ray::ray(const rt::vector& o, const rt::vector& d, const rt::color& c)
    : origin(o), direction(d), color(c) {}

ray::ray(const rt::vector& o, const rt::vector& d)
    : origin(o), direction(d), color(rt::color::WHITE) {}


ray::ray() {
    origin = rt::vector();
    direction = rt::vector();
    color = rt::color();
}

/* Accessors */

rt::vector ray::get_origin() const {
    return origin;
}

rt::vector ray::get_direction() const {
    return direction;
}

rt::color ray::get_color() const {
    return color;
}
