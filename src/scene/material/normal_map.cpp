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



