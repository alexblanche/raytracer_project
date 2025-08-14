#pragma once

#include <vector>
#include "light/vector.hpp"

class normal_map {

    private:
        int width, height;
        std::vector<std::vector<rt::vector>> data;
        real width_minus_one, height_minus_one;
    
    public:

        normal_map();

        normal_map(const unsigned int width, const unsigned int height, std::vector<std::vector<rt::vector>>&& data);

        /* Constructor from a .bmp file
           Writes true in parsing_successful if the operation was successful */
        normal_map(const char* file_name, bool& parsing_successful);

        normal_map(const normal_map&) = delete;

        normal_map& operator=(const normal_map&) = delete;

        normal_map(normal_map&&) = default;

        normal_map& operator=(normal_map&&) = default;

        /* Returns the normal in tangent space at the given UV-coordinates u, v (between 0 and 1) */
        /* Returns the local normal at the given UV-coordinates u, v (between 0 and 1) */
        inline const rt::vector& get_tangent_space_normal(const real u, const real v) const {
            const int x = u * width_minus_one;
            const int y = v * height_minus_one;
            // Due to floating-point imprecision, some "unit" vector have a norm slightly larger than 1,
            // producing out of range coordinates
            return (x < 0 || x >= width || y < 0 || y >= height) ?
                data[std::min(width - 1, std::max(0, x))][std::min(height - 1, std::max(0, y))]
                :
                data[x][y];
        }
};