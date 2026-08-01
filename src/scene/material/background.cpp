#include "scene/material/background.hpp"

#include "math/geometry/trigonometry.hpp"

#include <cmath>

/* Returns the color of the pixel dir is pointing at, when a texture is set */
const rt::color& background_container::get_texture_color(const rt::vector& dir) const {
    
    const rt::vector dir_rotated = rotation_matrix * dir;
    const auto& [ theta, phi ] = trig::get_angles(dir_rotated);

    /* Determining the UV-coordinates */
    const real u = 1.0_r - theta / (2.0_r * PI);
    const real v = 1.0_r - phi / PI;

    return bg_texture.get_color(u, v);
}