#include "headers/texture.hpp"
#include "../../file_readers/headers/bmp_reader.hpp"

#include <cmath>

#include <iostream>

std::vector<const texture*> texture::set;


/* Constructors */

/* Default constructor */
texture::texture(const int width, const int height, const std::vector<std::vector<rt::color>>& data)
    : width(width), height(height), data(data) {

    set.push_back(this);
}

/* Constructor from a .bmp file */
texture::texture(const char* file_name) {
    read_bmp_size(file_name, width, height);
    data = std::vector<std::vector<rt::color>>(width, std::vector<rt::color>(height));
    read_bmp(file_name, data);
    set.push_back(this);
}


/* Accessor */

/* Returns the color stored in data at UV-coordinates u, v between 0 and 1 times width, height */
rt::color texture::get_color(const double& u, const double& v) const {
    // printf("u = %lf, v = %lf, width = %d, height = %d, u * width = %f, v * height = %f data dimensions = %u %u\n",
    //     u, v, width, height, u * width, v * height, data.size(), data.at(0).size());
    rt::color c = data.at((int) (u * width)).at((int) (v * height));
    // printf("Color ready to be returned\n");
    return c;
}

