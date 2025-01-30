#pragma once

#include "light/vector.hpp"
#include "texture.hpp"
#include <optional>
#include <cmath>

#define PI 3.14159265358979323846f

/* Struct containing the background color, the background texture and its orientation */
struct background_container {
    rt::color bg_color;
    std::optional<texture> bg_texture;
    real rotate_x;
    real rotate_y;
    real rotate_z;

    background_container(const rt::color& col);

    background_container(texture&& txt, const real rx, const real ry, const real rz);
    
    inline bool has_texture() const {
        return bg_texture.has_value();
    }

    /* Returns the background color when it is a color */
    inline const rt::color& get_color() const {
        return bg_color;
    }

    /* Returns the color of the pixel dir is pointing at, when a texture is set */
    const rt::color& get_color(const rt::vector& dir) const;
};