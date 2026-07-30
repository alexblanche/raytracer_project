#pragma once

#include "scene/material/texture.hpp"

#include "math/geometry/mat3.hpp"

#include <cmath>

constexpr linalg::mat_type mat_type = linalg::mat_type::Col;

/* Struct containing the background color, the background texture and its orientation */
struct background_container {

    enum class type {
        Untextured, Textured
    };

    type type;
    rt::color bg_color;
    texture bg_texture;
    linalg::mat3<mat_type> rotation_matrix;

    /* Struct containing the background color, the background texture and its orientation */
    background_container(const rt::color& col)
        : type(type::Untextured), bg_color(col) {}

    background_container(texture&& txt, const real theta_x, const real theta_y, const real theta_z)
        : type(type::Textured), bg_texture(std::move(txt)),
            rotation_matrix(linalg::mat3<mat_type>::rotation(theta_x, theta_y, theta_z)) {}
    
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