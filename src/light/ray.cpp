#include "light/ray.hpp"
#include "screen/color.hpp"

#include <cmath>

/** 
 * The ray class contains the information of a ray of light:
 * the point of origin, the direction vector and the color.
*/

ray::ray() {}

ray::ray(const rt::vector& origin, const rt::vector& dir)
    : origin(origin), direction(dir),
      inv_dir(1.0_r / direction.x, 1.0_r / direction.y, 1.0_r / direction.z),
      abs_inv_dir(abs(inv_dir.x), abs(inv_dir.y), abs(inv_dir.z)) {}
