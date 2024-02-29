#pragma once

#include <vector>
#include "../../screen/headers/color.hpp"

/* Extracts the data from the given .bmp file: stores the width and height in the provided
   references, and returns a matrix of width rows and height columns containing colors  */
std::vector<std::vector<rt::color>> read_bmp(const char* file_name, int& width, int& height);

/* Writes the data into a .bmp file with the given name */
/* TODO
void write_bmp(const char* file_name, std::vector<std::vector<rt::color>>& data,
    const int width, const int height);
 */