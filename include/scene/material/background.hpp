#pragma once

#include "scene/material/texture.hpp"

#include <cmath>

constexpr bool ROTATION_MATRIX_COLUMNS = true; // true = columns, false = rows

/* Struct containing the background color, the background texture and its orientation */
struct background_container {

    enum class type {
        Untextured, Textured
    };

    type type;
    rt::color bg_color;
    texture bg_texture;
    rt::vector m1, m2, m3; // columns of the rotation matrix

    /* Struct containing the background color, the background texture and its orientation */
    background_container(const rt::color& col)
        : type(type::Untextured), bg_color(col) {}

    background_container(texture&& txt, const real theta_x, const real theta_y, const real theta_z)
        : type(type::Textured), bg_texture(std::move(txt))
                //theta_x(theta_x), theta_y(theta_y), theta_z(theta_z)
        {

        // Matrix product of rotation matrices of angles theta_x, theta_y, theta_z around axes x, y, z respectively
        const real cos_x = cos(theta_x);
        const real sin_x = sin(theta_x);
        
        const real cos_y = cos(theta_y);
        const real sin_y = sin(theta_y);
        
        const real cos_z = cos(theta_z);
        const real sin_z = sin(theta_z);

        // Columns of the rotation matrix
        if constexpr (ROTATION_MATRIX_COLUMNS) {
            m1 = rt::vector(
                cos_y * cos_z,
                cos_y * sin_z,
                -sin_y
            );
            m2 = rt::vector(
                cos_z * sin_x * sin_y - cos_x * sin_z,
                cos_x * cos_z + sin_x * sin_y * sin_z,
                cos_y * sin_x    
            );
            m3 = rt::vector(
                cos_x * cos_z * sin_y + sin_x * sin_z,
                cos_x * sin_y * sin_z - cos_z * sin_x,
                cos_x * cos_y
            );
        }
        else {
            // Rows of the rotation matrix
            m1 = rt::vector(
                cos_y * cos_z,
                cos_z * sin_x * sin_y - cos_x * sin_z,
                cos_x * cos_z * sin_y + sin_x * sin_z
            );
            m2 = rt::vector(
                cos_y * sin_z,
                cos_x * cos_z + sin_x * sin_y * sin_z,
                cos_x * sin_y * sin_z - cos_z * sin_x
            );
            m3 = rt::vector(
                -sin_y,
                cos_y * sin_x,
                cos_x * cos_y
            );
        }
    }
    
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