#pragma once

#include "scene/material/texture.hpp"

/* Struct containing the background color, the background texture and its orientation */
struct background_container {

    enum class type {
        Untextured, Textured
    };

    type type;
    rt::color bg_color;
    texture bg_texture;
    real rotate_x = 0.0_r;
    real rotate_y = 0.0_r;
    real rotate_z = 0.0_r;

    /* Struct containing the background color, the background texture and its orientation */
    background_container(const rt::color& col)
        : type(type::Untextured), bg_color(col) {}

    background_container(texture&& txt, const real rx, const real ry, const real rz)
        : type(type::Textured), bg_texture(std::move(txt)), rotate_x(rx), rotate_y(ry), rotate_z(rz) {}
    
    inline bool has_texture() const {
        return type == type::Textured;
    }

    /* Returns the background color when it is a color */
    inline const rt::color& get_color() const {
        return bg_color;
    }

    /* Returns the color of the pixel dir is pointing at, when a texture is set */
    const rt::color& get_color(const rt::vector& dir) const;
};