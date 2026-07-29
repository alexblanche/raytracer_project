#include "scene/material/material.hpp"
#include "screen/color.hpp"

#include <cmath>
#include <iostream>

/* Constructor from mtl parameters */
material::material(const real ns,
    const rt::color& ka, const rt::color& kd, const rt::color& ks, const rt::color& ke,
    const real ni, const real d, const unsigned int illum, std::optional<real> gamma)
    
    : color(kd * 255.0_r), smoothness(pow(ns / 1000.0_r, 0.25_r)),
      reflectivity(ks.get_average()),
      refraction_scattering(0.0_r), refraction_index(ni),
      reflects_color(false), has_refraction_index(ni != 1.0_r) {

    /* Ambient light is unused and left to global illumination
       reflects_color is left to false
       refraction_scattering is set equal to the reflectivity
    */
    has_specularity = reflectivity != 0;
    emissive = false;

    // Light emission
    if (ke == rt::BLACK) {
        emission_intensity = 0.0_r;
    }
    else {
        // Temporary
        emission_intensity = 10.0_r;
        emissive = true;
    }

    opaque = true;

    if (illum == 4 || illum == 6 || illum == 7 || illum == 9) {
        // Glass
        transparency = 1.0_r - d;
        color = ka * 255.0_r; // Usually kd = black for glass
        opaque = false;
    }
    else {
        // Other materials
        transparency = 0.0_r;
    }

    if (gamma.has_value())
        color.apply_gamma(gamma.value());
}