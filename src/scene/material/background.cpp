#include "scene/material/background.hpp"

#define PI 3.14159265358979323846f

/* Struct containing the background color, the background texture and its orientation */
background_container::background_container(const rt::color& col)
    : bg_color(col),
    bg_texture(std::nullopt), rotate_x(0), rotate_y(0), rotate_z(0) {}

background_container::background_container(texture&& txt, const real rx, const real ry, const real rz)
    : bg_color(rt::color(0, 0, 0)),
    bg_texture(std::move(txt)), rotate_x(rx), rotate_y(ry), rotate_z(rz) {}

/* Returns the color of the pixel dir is pointing at, when a texture is set */
const rt::color& background_container::get_color(const rt::vector& dir) const {

    rt::vector dir_rotated = dir;
    if (rotate_x != 0.0f) {
        dir_rotated = dir_rotated.rotate_x(rotate_x);
    }
    if (rotate_y != 0.0f) {
        dir_rotated = dir_rotated.rotate_y(rotate_y);
    }
    if (rotate_z != 0.0f) {
        dir_rotated = dir_rotated.rotate_z(rotate_z);
    }

    
    /* Determining the pixel of the background texture to display */      
    real phi = asinf(dir_rotated.y) + 0.5f * PI;
    // dir is a unit vector, but due to floating-point imprecision, dir.y can be greater than 1
    if (abs(dir_rotated.y) >= 1.0f) {
        phi = (dir_rotated.y > 0.0f) ? PI : 0;
    }

    real theta;
    if (dir_rotated.x > 0.0f) {
        theta = atanf(dir_rotated.z / dir_rotated.x) + 1.5f * PI;
    }
    else if (dir_rotated.x < 0.0f) {
        theta = atanf(dir_rotated.z / dir_rotated.x) + 0.5f * PI;
    }
    else {
        theta = 0.0f;
    }

    /* Determining the UV-coordinates */
    const real u = 1.0f - theta / (2.0f * PI);
    const real v = phi / PI;

    return bg_texture.value().get_color(u, v);
}