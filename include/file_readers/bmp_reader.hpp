#pragma once

#include <vector>
#include "screen/color.hpp"
#include <optional>

struct dimensions {
   int width, height;
   dimensions(int width, int height)
      : width(width), height(height) {}
};

/* Returns the width and height of the .bmp image contained in file_name */
std::optional<dimensions> read_bmp_size(const char* file_name);

/* Extracts the data from the given .bmp file: stores the width and height in the provided
   references, and returns a matrix of width rows and height columns containing colors
   Returns true if the operation was successful */
bool read_bmp(const char* file_name, std::vector<std::vector<rt::color>>& data);

/* Returns the integer stored in the buffer at index start_index on nb_bytes bytes (in Little Endian convention) */
unsigned int value_of_bytes(const unsigned char buffer[], int start_index, int nb_bytes);

/* Prints the info contained in the header of the given .bmp file */
bool print_bmp_info(const char* file_name);

/* Writes the data into a .bmp file with the given name
   The value (real) of each component of each color of data is divided by number_of_rays before being written in the file
   Returns true if the operation was successful */
bool write_bmp(const char* file_name, const std::vector<std::vector<rt::color>>& data, unsigned int number_of_rays, real gamma = 1.0f);
