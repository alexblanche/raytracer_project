#include "headers/ray.hpp"
#include "../screen/headers/color.hpp"
#include <cmath>

using namespace std;

/** 
 * The ray class contains the information of a ray of light:
 * the point of origin, the direction vector and the color.
*/

// vector<unsigned int> ray::obj_comp_cpt = {0, 0, 0, 0, 0};

/* Constructors */

ray::ray(const rt::vector& o, const rt::vector& d)
    : origin(o), direction(d) /*, origin_index(-1)*/ {
        inv_dir = rt::vector(1/d.x, 1/d.y, 1/d.z);
        abs_inv_dir = rt::vector(abs(inv_dir.x), abs(inv_dir.y), abs(inv_dir.z));
    }

ray::ray()
    : origin(rt::vector()), direction(rt::vector()), /* origin_index(-1), */
      inv_dir(rt::vector()) {}


/* Accessors */

rt::vector ray::get_origin() const {
    return origin;
}

rt::vector ray::get_direction() const {
    return direction;
}

rt::vector ray::get_inv_dir() const {
    return inv_dir;
}

rt::vector ray::get_abs_inv_dir() const {
    return abs_inv_dir;
}

// unsigned int ray::get_origin_index() const {
//     return origin_index;
// }

/* Mutators */

void ray::set_origin(const rt::vector& o) {
    origin = o;
}

void ray::set_direction(const rt::vector& d) {
    direction = d;
    inv_dir = rt::vector(1/d.x, 1/d.y, 1/d.z);
    abs_inv_dir = rt::vector(abs(inv_dir.x), abs(inv_dir.y), abs(inv_dir.z));
}

// void ray::set_origin_index(const unsigned int index) {
//     origin_index = index;
// }