#pragma once

#include <vector>
#include "../../screen/headers/color.hpp"

/* Writes in the variables the width and height of the .bmp image contained in file_name */
void read_bmp_size(const char* file_name, int& width, int& height);

/* Extracts the data from the given .bmp file: stores the width and height in the provided
   references, and returns a matrix of width rows and height columns containing colors  */
void read_bmp(const char* file_name, std::vector<std::vector<rt::color>>& data);

/* Writes the data into a .bmp file with the given name */
/* TODO
void write_bmp(const char* file_name, std::vector<std::vector<rt::color>>& data,
    const int width, const int height);
 */