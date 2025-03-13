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
texture::texture(const int width, const int height, const std::vector<std::vector<rt::color>>&& data)
    : width(width), height(height), data(std::move(data)),
        width_minus_one((real) (width - 1)), height_minus_one((real) (height - 1)) {}

/* Constructor from a .bmp or .hdr file
   Writes true in parsing_successful if the operation was successful */
texture::texture(const char* file_name, bool& parsing_successful, const real gamma) {

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
        if (gamma != 1.0f)
            apply_gamma(data, gamma);
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
