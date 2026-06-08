#include "scene/material/background.hpp"

#include <cmath>

/* Returns the color of the pixel dir is pointing at, when a texture is set */
const rt::color& background_container::get_color(const rt::vector& dir) const {

    rt::vector dir_rotated = dir;
    if (rotate_x != 0.0_r)
        dir_rotated = dir_rotated.rotate_x(rotate_x);
    if (rotate_y != 0.0_r)
        dir_rotated = dir_rotated.rotate_y(rotate_y);
    if (rotate_z != 0.0_r)
        dir_rotated = dir_rotated.rotate_z(rotate_z);

    
    /* Determining the pixel of the background texture to display */      
    real phi = asin(dir_rotated.y) + PI / 2.0_r;
    // dir is a unit vector, but due to floating-point imprecision, dir.y can be greater than 1
    if (abs(dir_rotated.y) >= 1.0_r)
        phi = (dir_rotated.y > 0.0_r) ? PI : 0.0_r;

    const real theta = (dir_rotated.x != 0.0_r) ?
          atan(dir_rotated.z / dir_rotated.x) + (dir_rotated.x > 0.0_r ? (3.0_r * PI / 2.0_r) : (PI / 2.0_r))
        : 0.0_r;

    /* Determining the UV-coordinates */
    const real u = 1.0_r - theta / (2.0_r * PI);
    const real v = 1.0_r - phi / PI;

    return bg_texture.get_color(u, v);
}