#include "light/ray.hpp"
#include "screen/color.hpp"
#include <cmath>

/** 
 * The ray class contains the information of a ray of light:
 * the point of origin, the direction vector and the color.
*/

/* Constructors */

ray::ray() {}

ray::ray(const rt::vector& o, const rt::vector& d)
    : origin(o), direction(d),
      inv_dir(1.0f / d.x, 1.0f / d.y, 1.0f / d.z),
      abs_inv_dir(std::abs(inv_dir.x), std::abs(inv_dir.y), std::abs(inv_dir.z)) {}

void ray::set_direction(const rt::vector& d) {
    direction = d;
    inv_dir = rt::vector(1.0f / d.x, 1.0f / d.y, 1.0f / d.z);
    abs_inv_dir = rt::vector(std::abs(inv_dir.x), std::abs(inv_dir.y), std::abs(inv_dir.z));
}
