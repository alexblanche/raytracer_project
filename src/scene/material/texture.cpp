#include "scene/material/texture.hpp"
#include "file_readers/bmp_reader.hpp"
#include "file_readers/hdr_reader.hpp"

#include <cmath>
#include <iostream>


/* Constructors */

texture::texture() {
    // printf("Creation of an empty texture\n");
}

/* Default constructor */
texture::texture(const int width, const int height, const std::vector<std::vector<rt::color>>& data)
    : width(width), height(height), data(data),
        width_minus_one((real) (width - 1)), height_minus_one((real) (height - 1)) {}

/* Constructor from a .bmp or .hdr file
   Writes true in parsing_successful if the operation was successful */
texture::texture(const char* file_name, bool& parsing_successful) {

    const char* p = file_name;
    while(*p != '\0')
        p++;
    p-=3;
    bool is_bmp;
    if (*p == 'b' && *(p+1) == 'm' && *(p+2) == 'p')
        is_bmp = true;
    else if (*p == 'h' && *(p+1) == 'd' && *(p+2) == 'r')
        is_bmp = false;
    else {
        printf("Error in texture definition: wrong file format\n");
        throw;
    }

    const std::optional<dimensions> dims = (is_bmp) ? read_bmp_size(file_name) : read_hdr_size(file_name);
    if (dims.has_value()) {
        width = dims.value().width;
        height = dims.value().height;
        data = std::vector<std::vector<rt::color>>(width, std::vector<rt::color>(height));
        bool read_success = (is_bmp) ? read_bmp(file_name, data) : read_hdr(file_name, data);
        width_minus_one = (real) (width - 1);
        height_minus_one = (real) (height - 1);
        parsing_successful = read_success;
    }
    else {
        parsing_successful = false;
    }

}


/* Accessor */

/* Returns the color stored in data at UV-coordinates u, v (between 0 and 1) times width, height */
const rt::color& texture::get_color(const real u, const real v) const {
    const int x = u * width_minus_one;
    const int y = v * height_minus_one;
    // Due to floating-point imprecision, some "unit" vector have a norm slightly larger than 1,
    // producing out of range coordinates
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return data[std::min(width - 1, std::max(0, x))][std::min(height - 1, std::max(0, y))];
    }
    else {
        return data[x][y];
    }
}


/*************************************************************************************************/

/** Texture infos **/

texture_info::texture_info()
    : texture_index((size_t) -1), uv_coordinates({}) {}

texture_info::texture_info(size_t texture_index, std::vector<real>&& uv_coordinates)
    : texture_index(texture_index), uv_coordinates(std::move(uv_coordinates)) {}

/* Texturing */

/* Write in u, v the UV-coordinate of the barycenter associated with the barycentric coordinates l1, l2
   In the case of quads, the boolean lower_triangle indicates that the three points to
   consider are (u0, v0), (u1, v1), (u2, v2) or (u0, v0), (u3, v3), (u2, v2) (in this order) */
uvcoord texture_info::get_barycenter(const barycentric_info& bary) const {

    if (uv_coordinates.size() == 6 || bary.lower_triangle) {
        // Triangles or Quads with (u0, v0), (u1, v1), (u2, v2) considered
        const real u = (1.0f - bary.l1 - bary.l2) * uv_coordinates[0] + bary.l1 * uv_coordinates[2] + bary.l2 * uv_coordinates[4];
        const real v = (1.0f - bary.l1 - bary.l2) * uv_coordinates[1] + bary.l1 * uv_coordinates[3] + bary.l2 * uv_coordinates[5];
        return uvcoord(u, v);
    }
    else {
        // Quads with (u0, v0), (u3, v3), (u2, v2) (in this order) considered
        const real u = (1.0f - bary.l1 - bary.l2) * uv_coordinates[0] + bary.l1 * uv_coordinates[6] + bary.l2 * uv_coordinates[4];
        const real v = (1.0f - bary.l1 - bary.l2) * uv_coordinates[1] + bary.l1 * uv_coordinates[7] + bary.l2 * uv_coordinates[5];
        return uvcoord(u, v);
    }
}


/* Returns the color of the pixel associated with UV-coordinates u, v */
const rt::color& texture_info::get_texture_color(const barycentric_info& bary,
    const std::vector<texture>& texture_set) const {
    
    const uvcoord uvc = get_barycenter(bary);
    return texture_set[texture_index].get_color(uvc.u, uvc.v);

    /* HERE: we can introduce texture filtering, with a factor by adding a
       random number between 0 and something like 0.2 to u, v, in order to
       blur the texture a little for the first bounce
       (instead I expect it to be heavily pixelated from up close) */
}