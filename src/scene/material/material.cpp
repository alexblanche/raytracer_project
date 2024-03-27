#include "scene/material/material.hpp"
#include "screen/color.hpp"

#include <vector>
#include <iostream>

/* Static element */

const material material::DIFFUSE = material(rt::color(255,255,255), 0);
const material material::MIRROR = material(rt::color(255,255,255), 1);
const material material::GLASS = material(rt::color(255,255,255), rt::color(0,0,0), 1, 0, 1, false, 0.6, 0, 1.52);
const material material::WATER = material(rt::color(255,255,255), rt::color(0,0,0), 1, 0, 1, false, 1, 0, 1.33);


/* Constructors */

material::material() : color(rt::color::WHITE), reflectivity(0),
    emitted_color(rt::color::WHITE), emission_intensity(0),
    specular_probability(1), reflects_color(true),
    transparency(0), refraction_index(1) {}

material::material(const rt::color& color, const double& reflectivity)

    : color(color), reflectivity(reflectivity),
        emission_intensity(0),
        specular_probability(1), reflects_color(true),
        transparency(0), refraction_index(1) {}

material::material(const rt::color& color, const rt::color& emitted_color,
    const double& reflectivity, const double& emission_intensity,
    const double& specular_probability, const bool reflects_color,
    const double& transparency, const double& refraction_scattering,
    const double& refraction_index)

    : color(color), reflectivity(reflectivity),
        emitted_color(emitted_color), emission_intensity(emission_intensity),
        specular_probability(specular_probability), reflects_color(reflects_color),
        transparency(transparency), refraction_scattering(refraction_scattering),
        refraction_index(refraction_index) {}

/* Constructor from mtl parameters */
material::material(const double& ns,
    const rt::color& /*ka*/, const rt::color& kd, const rt::color& ks, const rt::color& ke,
    const double& ni, const double& d, const unsigned int illum)
    
    : color(kd * 255), reflectivity(ns / 1000),
      emitted_color(ke * 255),
      specular_probability((ks.get_red() + ks.get_green() + ks.get_blue()) / (3 * 255)),
      reflects_color(false),
      refraction_scattering(ns / 1000), refraction_index(ni) {

    /* Ambient light is unused and left to global illumination
       reflects_color is left to false
       refraction_scattering is set equal to the reflectivity
    */

    // Light emission
    if (ke == rt::color(0, 0, 0)) {
        emission_intensity = 0;
    }
    else {
        emission_intensity = 1;
    }

    if (illum == 4 || illum == 6 || illum == 7 || illum == 9) {
        // Glass
        transparency = 1 - d;
    }
    else {
        // Other materials
        transparency = 0;
    }
}


/* Specific constructors */

/* Returns a diffuse material of given color */
material diffuse_material(const rt::color& color) {
    return material(color, 0);
}

/* Returns a light of given color and intensity */
material light_material(const rt::color& color, const double& emission_intensity) {
    return material(color, color, 0, emission_intensity, 0, false, 0, 0, 1);
}