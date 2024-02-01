#include "headers/material.hpp"
#include "../../screen/headers/color.hpp"

/* Constructors */

material::material() : color(rt::color::WHITE), emitted_color(rt::color::WHITE),
    reflectivity(0), emission_intensity(0) {}

material::material(const rt::color& color, const rt::color& emitted_color,
    const double reflectivity, const double emission_intensity)

    : color(color), emitted_color(emitted_color),
    reflectivity(reflectivity), emission_intensity(emission_intensity) {}


/* Accessors */

rt::color material::get_color() const {
    return color;
};

rt::color material::get_emitted_color() const {
    return emitted_color;
}

double material::get_reflectivity() const {
    return reflectivity;
}

double material::get_emission_intensity() const {
    return emission_intensity;
}