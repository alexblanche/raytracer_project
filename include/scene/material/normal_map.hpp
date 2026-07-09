#pragma once

#include "light/vector.hpp"

#include <string>
#include <vector>
#include <algorithm>

class normal_map {

    public:
        using vector_matrix = std::vector<std::vector<rt::vector>>;

    private:
        vector_matrix data;
        int width, height;
        real width_real, height_real;
    
    public:

        normal_map() {};

        normal_map(unsigned int width, unsigned int height, vector_matrix&& data);

        /* Constructor from a .bmp file
           Writes true in parsing_successful if the operation was successful */
        normal_map(const std::string& file_name, bool& parsing_successful);

        normal_map(normal_map&&)            noexcept = default;
        normal_map& operator=(normal_map&&) noexcept = default;

        normal_map(const normal_map&)               = delete;
        normal_map& operator=(const normal_map&)    = delete;

        /* Returns the normal in tangent space at the given UV-coordinates u, v (between 0 and 1) */
        /* Returns the local normal at the given UV-coordinates u, v (between 0 and 1) */
        inline const rt::vector& get_tangent_space_normal(const real u, const real v) const {
            const int x = u * width_real;
            const int y = v * height_real;
            // Due to floating-point imprecision, some "unit" vector have a norm slightly larger than 1,
            // producing out of range coordinates
            return data[ std::clamp(y, 0, height) ][ std::clamp(x, 0, width)]; 
        }
};