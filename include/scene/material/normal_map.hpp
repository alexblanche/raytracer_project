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
        const rt::vector& get_tangent_space_normal(const real u, const real v) const;
};