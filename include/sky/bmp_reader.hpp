#pragma once


#include "sky/color.hpp"
#include "auxiliary/exit_status.hpp"

#include <optional>
#include <vector>

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
exit_status read_bmp(const char* file_name, std::vector<std::vector<sky::color>>& data);

/* Prints the info contained in the header of the given .bmp file */
exit_status print_bmp_info(const char* file_name);