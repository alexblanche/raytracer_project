#include "headers/material.hpp"
#include "../../screen/headers/color.hpp"

//#include <fstream>

/* Static element */

const material material::MIRROR = material(rt::color(255,255,255), 1);


/* Constructors */

material::material() : color(rt::color::WHITE), emitted_color(rt::color::WHITE),
    reflectivity(0), emission_intensity(0), specular_probability(0), reflects_color(false) {}

material::material(const rt::color& color, const double reflectivity)
    : color(color), reflectivity(reflectivity), emission_intensity(0),
        specular_probability(0), reflects_color(false) {}

material::material(const rt::color& color, const rt::color& emitted_color,
    const double reflectivity, const double emission_intensity,
    const double specular_probability, const bool reflects_color)

    : color(color), emitted_color(emitted_color),
        reflectivity(reflectivity), emission_intensity(emission_intensity),
        specular_probability(specular_probability), reflects_color(reflects_color) {}


/* Specific constructors */

/* Returns a diffuse material of given color */
material diffuse_material(const rt::color& color) {
    return material(color, 0);
}

/* Returns a light of given color and intensity */
material light_material(const rt::color& color, const double emission_intensity) {
    return material(color, color, 0, emission_intensity, 0, false);
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

double material::get_specular_proba() const {
    return specular_probability;
}

bool material::does_reflect_color() const {
    return reflects_color;
}