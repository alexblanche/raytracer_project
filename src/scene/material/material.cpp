#include "headers/material.hpp"
#include "../../screen/headers/color.hpp"

/* Static element */
//const material material::MIRROR = material(rt::color::WHITE, rt::color::WHITE, 1, 0);

/* Constructors */

material::material() : color(rt::color::WHITE), emitted_color(rt::color::WHITE),
    reflectivity(0), emission_intensity(0) {}

material::material(const rt::color& color, const rt::color& emitted_color,
    const double reflectivity, const double emission_intensity)

    : color(color), emitted_color(emitted_color),
        reflectivity(reflectivity), emission_intensity(emission_intensity) {}







/* Specific constructors */

/* Returns a diffuse material of given color */
material diffuse_material(const rt::color& color) {
    return material(color, rt::color::WHITE, 0, 0);
}

/* Returns a light of given color and intensity */
material light_material(const rt::color& color, const double emission_intensity) {
    return material(color, color, 0, emission_intensity);
}


/* Accessors */

rt::color material::get_color() const {
    return color;
}

rt::color material::get_emitted_color() const {
    return emitted_color;
}

double material::get_reflectivity() const {
    return reflectivity;
}

double material::get_emission_intensity() const {
    return emission_intensity;
}