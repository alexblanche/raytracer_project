#include "scene/material/normal_map.hpp"

#include "file_readers/normal_map_reader.hpp"

normal_map::normal_map() {}

normal_map::normal_map(const unsigned int width, const unsigned int height, std::vector<std::vector<rt::vector>>&& data)
    : width(width), height(height), data(std::move(data)),
    width_minus_one((real) (width - 1)), height_minus_one((real) (height - 1)) {}


/* Constructor from a .bmp file
   Writes true in parsing_successful if the operation was successful */
normal_map::normal_map(const char* file_name, bool& parsing_successful) {

    parsing_successful = read_normal_map(file_name, data);
    
    if (parsing_successful) {
        width = data.size();
        height = data[0].size();
        width_minus_one = (real) (width - 1);
        height_minus_one = (real) (height - 1);
    }
}


/* Returns the local normal at the given UV-coordinates u, v (between 0 and 1) */
const rt::vector& normal_map::get_local_normal(const real u, const real v) const {
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
