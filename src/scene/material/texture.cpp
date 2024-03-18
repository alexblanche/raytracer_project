#include "scene/material/texture.hpp"
#include "file_readers/bmp_reader.hpp"

#include <cmath>
#include <iostream>


/* Constructors */

/* Default constructor */
texture::texture(const int width, const int height, const std::vector<std::vector<rt::color>>& data)
    : width(width), height(height), data(data),
        width_minus_one((double) (width - 1)), height_minus_one((double) (height - 1)) {}

/* Constructor from a .bmp file
   Writes true in parsing_successful if the operation was successful */
texture::texture(const char* file_name, bool& parsing_successful) {
    const bool read_size_success = read_bmp_size(file_name, width, height);
    data = std::vector<std::vector<rt::color>>(width, std::vector<rt::color>(height));
    const bool read_bmp_success = read_bmp(file_name, data);
    width_minus_one = (double) (width - 1);
    height_minus_one = (double) (height - 1);
    parsing_successful = read_size_success && read_bmp_success;
}


/* Accessor */

/* Returns the color stored in data at UV-coordinates u, v between 0 and 1 times width, height */
rt::color texture::get_color(const double& u, const double& v) const {
    return data.at((int) (u * width_minus_one)).at((int) (v * height_minus_one));
}


/*************************************************************************************************/

/** Texture infos **/

texture_info::texture_info()
    : texture_index((unsigned int) -1), uv_coordinates({}) {}

texture_info::texture_info(unsigned int texture_index, std::vector<double> uv_coordinates)
    : texture_index(texture_index), uv_coordinates(uv_coordinates) {}

/* Texturing */

/* Write in u, v the UV-coordinate of the barycenter associated with the barycentric coordinates l1, l2
   In the case of quads, the boolean lower_triangle indicates that the three points to
   consider are (u0, v0), (u1, v1), (u2, v2) or (u0, v0), (u3, v3), (u2, v2) (in this order) */
void texture_info::get_barycenter(const double& l1, const double& l2, const bool lower_triangle,
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
rt::color texture_info::get_texture_color(const double& l1, const double& l2, const bool lower_triangle,
    const std::vector<texture>& texture_set) const {
    
    double u, v;
    get_barycenter(l1, l2, lower_triangle, u, v);
    return texture_set.at(texture_index).get_color(u, v);

    /* HERE: we can introduce texture filtering, with a factor by adding a
       random number between 0 and something like 0.2 to u, v, in order to
       blur the texture a little for the first bounce (instead I expect it to be heavily pixelated) */
}