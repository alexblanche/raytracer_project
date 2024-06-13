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
    real init_theta;
    real init_phi;

    background_container(const rt::color& col)
        : bg_color(col),
        bg_texture(std::nullopt), init_theta(0), init_phi(0) {}

    background_container(texture&& txt, const real& theta, const real& phi)
        : bg_color(rt::color(0, 0, 0)),
        bg_texture(std::move(txt)), init_theta(theta), init_phi(phi) {}

    inline bool has_texture() const {
        return bg_texture.has_value();
    }

    /* Returns the background color when it is a color */
    inline rt::color get_color() const {
        return bg_color;
    }

    /* Returns the color of the pixel dir is pointing at, when a texture is set */
    rt::color get_color(const rt::vector& dir) const {
        
        /* Determining the pixel of the background texture to display */      
        real phi = asinf(dir.y) + 0.5f * PI;
        // dir is a unit vector, but due to floating-point imprecision, dir.y can be greater than 1
        if (abs(dir.y) >= 1.0f) {
            phi = (dir.y > 0.0f) ? PI : 0;
        }

        real theta;
        if (dir.x > 0.0f) {
            theta = atanf(dir.z / dir.x) + 1.5f * PI;
        }
        else if (dir.x < 0.0f) {
            theta = atanf(dir.z / dir.x) + 0.5f * PI;
        }
        else {
            theta = 0.0f;
        }

        /* Determining the UV-coordinates, taking the initial horizontal and vertical angles into account */
        real u = (theta + init_theta) / (2.0f * PI);
        if (u > 1.0f) { u -= 1.0f; }

        real v = (phi - init_phi) / PI;
        if (v < 0.0f) {
            v = - v;
            u += 0.5f;
            if (u > 1.0f) { u -= 1.0f; }
        }
        else if (v > 1.0f) {
            v = 2.0f - v;
            u += 0.5f;
            if (u > 1.0f) { u -= 1.0f; }
        }

        return bg_texture.value().get_color(u, v);
    }
};