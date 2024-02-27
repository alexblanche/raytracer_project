#include "headers/material.hpp"
#include "../../screen/headers/color.hpp"

//#include <fstream>

/* Static element */

const material material::MIRROR = material(rt::color(255,255,255), 1);
const material material::GLASS = material(rt::color(255,255,255), rt::color(0,0,0), 1, 0, 1, false, 1, 0, 1.3);
// const material material::WATER = material();


/* Constructors */

material::material() : color(rt::color::WHITE), emitted_color(rt::color::WHITE),
    reflectivity(0), emission_intensity(0), specular_probability(1), reflects_color(true),
    transparency(0), refraction_index(1) {}

material::material(const rt::color& color, const double reflectivity)

    : color(color), reflectivity(reflectivity), emission_intensity(0),
        specular_probability(1), reflects_color(true),
        transparency(0), refraction_index(1) {}

material::material(const rt::color& color, const rt::color& emitted_color,
    const double& reflectivity, const double& emission_intensity,
    const double& specular_probability, const bool reflects_color,
    const double& transparency, const double& refraction_scattering,
    const double& refraction_index)

    : color(color), emitted_color(emitted_color),
        reflectivity(reflectivity), emission_intensity(emission_intensity),
        specular_probability(specular_probability), reflects_color(reflects_color),
        transparency(transparency), refraction_scattering(refraction_scattering),
        refraction_index(refraction_index) {}


/* Specific constructors */

/* Returns a diffuse material of given color */
material diffuse_material(const rt::color& color) {
    return material(color, 0);
}

/* Returns a light of given color and intensity */
material light_material(const rt::color& color, const double emission_intensity) {
    return material(color, color, 0, emission_intensity, 0, false, 0, 0, 1);
}