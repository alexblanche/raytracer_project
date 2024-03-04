#include "light/ray.hpp"
#include "screen/color.hpp"
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


void ray::set_direction(const rt::vector& d) {
    direction = d;
    inv_dir = rt::vector(1/d.x, 1/d.y, 1/d.z);
    abs_inv_dir = rt::vector(abs(inv_dir.x), abs(inv_dir.y), abs(inv_dir.z));
}

// void ray::set_origin_index(const unsigned int index) {
//     origin_index = index;
// }