#include "headers/ray.hpp"
#include "../screen/headers/color.hpp"

using namespace std;

/** 
 * The ray class contains the information of a ray of light:
 * the point of origin, the direction vector and the color.
*/

/* Constructors */

ray::ray(const rt::vector& o, const rt::vector& d)
    : origin(o), direction(d), origin_index(-1) {}

ray::ray()
    : origin(rt::vector()), direction(rt::vector()), origin_index(-1) {}


/* Accessors */

rt::vector ray::get_origin() const {
    return origin;
}

rt::vector ray::get_direction() const {
    return direction;
}

unsigned int ray::get_origin_index() const {
    return origin_index;
}

/* Mutators */

void ray::set_origin(const rt::vector& o) {
    origin = o;
}

void ray::set_direction(const rt::vector& d) {
    direction = d;
}

void ray::set_origin_index(const unsigned int index) {
    origin_index = index;
}