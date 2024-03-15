#include "scene/material/material.hpp"
#include "scene/material/texture.hpp"
#include "screen/color.hpp"

#include <vector>
#include <iostream>

/* Static element */

const material material::DIFFUSE = material(rt::color(255,255,255), 0);
const material material::MIRROR = material(rt::color(255,255,255), 1);
const material material::GLASS = material(rt::color(255,255,255), rt::color(0,0,0), 1, 0, 1, false, 0.2, 0, 1.3);
const material material::WATER = material(rt::color(255,255,255), rt::color(0,0,0), 1, 0, 1, false, 1, 0, 1.5);


/* Constructors */

material::material() : color(rt::color::WHITE), reflectivity(0),
    emitted_color(rt::color::WHITE), emission_intensity(0),
    specular_probability(1), reflects_color(true),
    transparency(0), refraction_index(1),
    textured(false) {}

material::material(const rt::color& color, const double& reflectivity)

    : color(color), reflectivity(reflectivity),
        emission_intensity(0),
        specular_probability(1), reflects_color(true),
        transparency(0), refraction_index(1),
        textured(false) {}

material::material(const rt::color& color, const rt::color& emitted_color,
    const double& reflectivity, const double& emission_intensity,
    const double& specular_probability, const bool reflects_color,
    const double& transparency, const double& refraction_scattering,
    const double& refraction_index)

    : color(color), reflectivity(reflectivity),
        emitted_color(emitted_color), emission_intensity(emission_intensity),
        specular_probability(specular_probability), reflects_color(reflects_color),
        transparency(transparency), refraction_scattering(refraction_scattering),
        refraction_index(refraction_index),
        textured(false) {}

material::material(const rt::color& color, const rt::color& emitted_color,
    const double& reflectivity, const double& emission_intensity,
    const double& specular_probability, const bool reflects_color,
    const double& transparency, const double& refraction_scattering,
    const double& refraction_index,
    const unsigned int texture_index, const std::vector<double>& uv_coordinates)

    : color(color), reflectivity(reflectivity),
        emitted_color(emitted_color), emission_intensity(emission_intensity),
        specular_probability(specular_probability), reflects_color(reflects_color),
        transparency(transparency), refraction_scattering(refraction_scattering),
        refraction_index(refraction_index),
        textured(true), texture_index(texture_index), uv_coordinates(uv_coordinates) {}

material::material(const material& m, const unsigned int texture_index, const std::vector<double>& uv_coordinates)
    
    : color(m.color), reflectivity(m.reflectivity),
        emitted_color(m.emitted_color), emission_intensity(m.emission_intensity),
        specular_probability(m.specular_probability), reflects_color(m.reflects_color),
        transparency(m.transparency), refraction_scattering(m.refraction_scattering),
        refraction_index(m.refraction_index),
        textured(true), texture_index(texture_index), uv_coordinates(uv_coordinates) {}


/* Specific constructors */

/* Returns a diffuse material of given color */
material diffuse_material(const rt::color& color) {
    return material(color, 0);
}

/* Returns a light of given color and intensity */
material light_material(const rt::color& color, const double& emission_intensity) {
    return material(color, color, 0, emission_intensity, 0, false, 0, 0, 1);
}


/* Texturing */

/* Write in u, v the UV-coordinate of the barycenter associated with the barycentric coordinates l1, l2
   In the case of quads, the boolean lower_triangle indicates that the three points to
   consider are (u0, v0), (u1, v1), (u2, v2) or (u0, v0), (u3, v3), (u2, v2) (in this order) */
void material::get_barycenter(const double& l1, const double& l2, const bool lower_triangle,
    double& u, double& v) const {

    if (uv_coordinates.size() == 6 || lower_triangle) {
        // Triangles or Quads with (u0, v0), (u1, v1), (u2, v2) considered
        u = (1 - l1 - l2) * uv_coordinates.at(0) + l1 * uv_coordinates.at(2) + l2 * uv_coordinates.at(4);
        v = (1 - l1 - l2) * uv_coordinates.at(1) + l1 * uv_coordinates.at(3) + l2 * uv_coordinates.at(5);
    }
    else {
        // Quads with (u0, v0), (u3, v3), (u2, v2) (in this order) considered
        u = (1 - l1 - l2) * uv_coordinates.at(0) + l1 * uv_coordinates.at(6) + l2 * uv_coordinates.at(4);
        v = (1 - l1 - l2) * uv_coordinates.at(1) + l1 * uv_coordinates.at(7) + l2 * uv_coordinates.at(5);
    }
}


/* Returns the color of the pixel associated with UV-coordinates u, v */
rt::color material::get_texture_color(const double& l1, const double& l2, const bool lower_triangle,
    const std::vector<const texture*>& texture_set) const {
    
    double u, v;
    get_barycenter(l1, l2, lower_triangle, u, v);
    return texture_set.at(texture_index)->get_color(u, v);

    /* HERE: we can introduce texture filtering, with a factor by adding a
       random number between 0 and something like 0.2 to u, v, in order to
       blur the texture a little for the first bounce (instead I expect it to be heavily pixelated) */
}