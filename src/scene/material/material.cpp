#include "scene/material/material.hpp"
#include "screen/color.hpp"

#include <cmath>

#include <vector>
#include <iostream>

/* Static element */

material material::DIFFUSE = material(rt::color(255,255,255), 0);
material material::MIRROR = material(rt::color(255,255,255), rt::color(0,0,0), 1, 0, 1, false, 0, 0, 1.0);
material material::GLASS = material(rt::color(255,255,255), rt::color(0,0,0), 1, 0, 1, false, 0.95, 0, 1.52);
material material::WATER = material(rt::color(255,255,255), rt::color(0,0,0), 1, 0, 1, false, 1, 0, 1.33);


/* Constructors */

material::material() : color(rt::color::WHITE), reflectivity(0),
    emitted_color(rt::color::WHITE), emission_intensity(0),
    specular_probability(0), reflects_color(false),
    transparency(0), refraction_index(1),
    opaque(true), emissive(false), has_spec_prob(false) {}

material::material(const rt::color& color, const real reflectivity)

    : color(color), reflectivity(reflectivity),
        emission_intensity(0),
        specular_probability(0), reflects_color(false),
        transparency(0), refraction_index(1),
        opaque(true), emissive(false), has_spec_prob(false) {}

material::material(const rt::color& color, const rt::color& emitted_color,
    const real reflectivity, const real emission_intensity,
    const real specular_probability, const bool reflects_color,
    const real transparency, const real refraction_scattering,
    const real refraction_index)

    : color(color), reflectivity(reflectivity),
        emitted_color(emitted_color), emission_intensity(emission_intensity),
        specular_probability(specular_probability), reflects_color(reflects_color),
        transparency(transparency), refraction_scattering(refraction_scattering),
        refraction_index(refraction_index),
        opaque(transparency == 0), emissive(emission_intensity != 0), has_spec_prob(specular_probability != 0) {}

/* Constructor from mtl parameters */
material::material(const real ns,
    const rt::color& ka, const rt::color& kd, const rt::color& ks, const rt::color& ke,
    const real ni, const real d, const unsigned int illum, const real gamma)
    
    : color(kd * 255), reflectivity(pow(ns / 1000, 0.25)),
      emitted_color(ke * 255),
      specular_probability((ks.get_red() + ks.get_green() + ks.get_blue()) / 3),
      reflects_color(false),
      refraction_scattering(0), refraction_index(ni) {

    /* Ambient light is unused and left to global illumination
       reflects_color is left to false
       refraction_scattering is set equal to the reflectivity
    */
    has_spec_prob = specular_probability != 0;
    emissive = false;

    // Light emission
    if (ke == rt::color(0, 0, 0)) {
        emission_intensity = 0;
    }
    else {
        // Temporary
        emission_intensity = 10;
        emissive = true;
    }

    opaque = true;

    if (illum == 4 || illum == 6 || illum == 7 || illum == 9) {
        // Glass
        transparency = 1 - d;
        color = ka * 255; // Usually kd = black for glass
        opaque = false;
    }
    else {
        // Other materials
        transparency = 0;
    }

    if (gamma != 1.0f) {
        const real gr = pow(color.get_red()   / 255.0f, gamma) * 255.0f;
        const real gg = pow(color.get_green() / 255.0f, gamma) * 255.0f;
        const real gb = pow(color.get_blue()  / 255.0f, gamma) * 255.0f;
        color = rt::color(gr, gg, gb);

        const real ger = pow(emitted_color.get_red()   / 255.0f, gamma) * 255.0f;
        const real geg = pow(emitted_color.get_green() / 255.0f, gamma) * 255.0f;
        const real geb = pow(emitted_color.get_blue()  / 255.0f, gamma) * 255.0f;
        emitted_color = rt::color(ger, geg, geb);
    }
}


/* Specific constructors */

/* Returns a diffuse material of given color */
material diffuse_material(const rt::color& color) {
    return material(color, 0);
}

/* Returns a light of given color and intensity */
material light_material(const rt::color& color, const real emission_intensity) {
    return material(color, color, 0, emission_intensity, 0, false, 0, 0, 1);
}