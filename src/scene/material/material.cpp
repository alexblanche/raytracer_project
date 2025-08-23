#include "scene/material/material.hpp"
#include "screen/color.hpp"

#include <cmath>

#include <vector>
#include <iostream>

/* Static element */

material material::DIFFUSE = material();
material material::MIRROR = material(rt::color(255,255,255), 1, 0, 1, false, 0, 0, 1.0);
material material::GLASS = material(rt::color(255,255,255), 1, 0, 1, false, 0.95, 0, 1.52);
material material::WATER = material(rt::color(255,255,255), 1, 0, 1, false, 1, 0, 1.33);


/* Constructors */

material::material() : color(rt::color::WHITE), smoothness(0),
    emission_intensity(0),
    reflectivity(0),
    transparency(0), refraction_index(1),
    opaque(true), emissive(false), has_specularity(false), reflects_color(false) {}

material::material(const rt::color& color, const real smoothness)

    : color(color), smoothness(smoothness),
        emission_intensity(0),
        reflectivity(0),
        transparency(0), refraction_index(1),
        opaque(true), emissive(false), has_specularity(false), 
        reflects_color(false) {}

material::material(const rt::color& color,
    const real smoothness, const real emission_intensity,
    const real reflectivity, const bool reflects_color,
    const real transparency, const real refraction_scattering,
    const real refraction_index)

    : color(color), smoothness(smoothness),
        emission_intensity(emission_intensity),
        reflectivity(reflectivity),
        transparency(transparency), refraction_scattering(refraction_scattering),
        refraction_index(refraction_index),
        opaque(transparency == 0), emissive(emission_intensity != 0), has_specularity(reflectivity != 0),
        reflects_color(reflects_color) {
        
            // printf("material: color:(%d, %d, %d), smooth:%lf, refl:%lf, has_spec:%d, refl_col:%d\n",
            //     (int) color.get_red(), (int) color.get_green(), (int) color.get_blue(), smoothness, reflectivity, has_specularity, reflects_color);
        }

/* Constructor from mtl parameters */
material::material(const real ns,
    const rt::color& ka, const rt::color& kd, const rt::color& ks, const rt::color& ke,
    const real ni, const real d, const unsigned int illum, const real gamma)
    
    : color(kd * 255), smoothness(pow(ns / 1000, 0.25)),
      //emitted_color(ke * 255),
      reflectivity(ks.get_average()),
      refraction_scattering(0), refraction_index(ni),
      reflects_color(false) {

    /* Ambient light is unused and left to global illumination
       reflects_color is left to false
       refraction_scattering is set equal to the reflectivity
    */
    has_specularity = reflectivity != 0;
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

        // const real ger = pow(emitted_color.get_red()   / 255.0f, gamma) * 255.0f;
        // const real geg = pow(emitted_color.get_green() / 255.0f, gamma) * 255.0f;
        // const real geb = pow(emitted_color.get_blue()  / 255.0f, gamma) * 255.0f;
        // emitted_color = rt::color(ger, geg, geb);
    }
}


/* Specific constructors */

/* Returns a diffuse material of given color */
material diffuse_material(const rt::color& color) {
    return material(color, 0);
}

/* Returns a light of given color and intensity */
material light_material(const rt::color& color, const real emission_intensity) {
    return material(color, 0, emission_intensity, 0, false, 0, 0, 1);
}