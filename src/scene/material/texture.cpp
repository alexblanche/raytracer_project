#include "scene/material/texture.hpp"
#include "file_readers/bmp_reader.hpp"

#include <cmath>
#include <iostream>

std::vector<const texture*> texture::set;


/* Constructors */

/* Default constructor */
texture::texture(const int width, const int height, const std::vector<std::vector<rt::color>>& data)
    : width(width), height(height), data(data),
        width_minus_one((double) (width - 1)), height_minus_one((double) height_minus_one) {

    set.push_back(this);
}

/* Constructor from a .bmp file */
texture::texture(const char* file_name) {
    read_bmp_size(file_name, width, height);
    data = std::vector<std::vector<rt::color>>(width, std::vector<rt::color>(height));
    read_bmp(file_name, data);
    set.push_back(this);
    width_minus_one = (double) (width - 1);
    height_minus_one = (double) (height - 1);
}


/* Accessor */

/* Returns the color stored in data at UV-coordinates u, v between 0 and 1 times width, height */
rt::color texture::get_color(const double& u, const double& v) const {
    return data.at((int) (u * width_minus_one)).at((int) (v * height_minus_one));
}

