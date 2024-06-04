#include "scene/material/texture.hpp"
#include "file_readers/bmp_reader.hpp"

#include <cmath>
#include <iostream>


/* Constructors */

texture::texture() {
    printf("Creating of an empty texture\n");
}

/* Default constructor */
texture::texture(const int width, const int height, const std::vector<std::vector<rt::color>>& data)
    : width(width), height(height), data(data),
        width_minus_one((double) (width - 1)), height_minus_one((double) (height - 1)) {

    printf("Creating of a texture from the main constructor\n");
}

/* Constructor from a .bmp file
   Writes true in parsing_successful if the operation was successful */
texture::texture(const char* file_name, bool& parsing_successful) {

    printf("Creating of a texture from a file\n");

    const std::optional<dimensions> dims = read_bmp_size(file_name);
    if (dims.has_value()) {
        width = dims.value().width;
        height = dims.value().height;
        data = std::vector<std::vector<rt::color>>(width, std::vector<rt::color>(height));
        const bool read_bmp_success = read_bmp(file_name, data);
        width_minus_one = (double) (width - 1);
        height_minus_one = (double) (height - 1);
        parsing_successful = read_bmp_success;
    }
    else {
        parsing_successful = false;
    }
    
}


/* Accessor */

/* Returns the color stored in data at UV-coordinates u, v between 0 and 1 times width, height */
rt::color texture::get_color(const double& u, const double& v) const {
    return data[(int) (u * width_minus_one)][(int) (v * height_minus_one)];
}


/*************************************************************************************************/

/** Texture infos **/

texture_info::texture_info()
    : texture_index((size_t) -1), uv_coordinates({}) {}

texture_info::texture_info(size_t texture_index, std::vector<double>&& uv_coordinates)
    : texture_index(texture_index), uv_coordinates(std::move(uv_coordinates)) {}

/* Texturing */

/* Write in u, v the UV-coordinate of the barycenter associated with the barycentric coordinates l1, l2
   In the case of quads, the boolean lower_triangle indicates that the three points to
   consider are (u0, v0), (u1, v1), (u2, v2) or (u0, v0), (u3, v3), (u2, v2) (in this order) */
uvcoord texture_info::get_barycenter(const barycentric_info& bary) const {

    if (uv_coordinates.size() == 6 || bary.lower_triangle) {
        // Triangles or Quads with (u0, v0), (u1, v1), (u2, v2) considered
        const double u = (1 - bary.l1 - bary.l2) * uv_coordinates[0] + bary.l1 * uv_coordinates[2] + bary.l2 * uv_coordinates[4];
        const double v = (1 - bary.l1 - bary.l2) * uv_coordinates[1] + bary.l1 * uv_coordinates[3] + bary.l2 * uv_coordinates[5];
        return uvcoord(u, v);
    }
    else {
        // Quads with (u0, v0), (u3, v3), (u2, v2) (in this order) considered
        const double u = (1 - bary.l1 - bary.l2) * uv_coordinates[0] + bary.l1 * uv_coordinates[6] + bary.l2 * uv_coordinates[4];
        const double v = (1 - bary.l1 - bary.l2) * uv_coordinates[1] + bary.l1 * uv_coordinates[7] + bary.l2 * uv_coordinates[5];
        return uvcoord(u, v);
    }
}


/* Returns the color of the pixel associated with UV-coordinates u, v */
rt::color texture_info::get_texture_color(const barycentric_info& bary,
    const std::vector<texture>& texture_set) const {
    
    const uvcoord uvc = get_barycenter(bary);
    return texture_set[texture_index].get_color(uvc.u, uvc.v);

    /* HERE: we can introduce texture filtering, with a factor by adding a
       random number between 0 and something like 0.2 to u, v, in order to
       blur the texture a little for the first bounce
       (instead I expect it to be heavily pixelated from up close) */
}