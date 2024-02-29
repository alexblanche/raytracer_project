#include "headers/texture.hpp"
#include "../../file_readers/headers/bmp_reader.hpp"

#include <cmath>

std::vector<const texture*> texture::set;


/* Constructors */

/* Default constructor */
texture::texture(const int width, const int height, const std::vector<std::vector<rt::color>>& data)
    : width(width), height(height), data(data) {

    set.push_back(this);
}

/* Constructor from a .bmp file */
texture::texture(const char* file_name) {
    data = read_bmp(file_name, width, height);
    set.push_back(this);
}


/* Accessor */

/* Returns the color stored in data at coordinates x, y between 0 and 1 times width, height */
rt::color texture::get_color(const double& x, const double& y) const {
    data.at((int) x * width).at((int) y * height);
}