#pragma once

#include "math/geometry/vector.hpp"
#include "file_readers/parsers/parsing_wrappers.hpp"

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
        normal_map() {}

        normal_map(unsigned int w, unsigned int h, vector_matrix&& data);

        /* Constructor from a .bmp file */
        normal_map(const std::string& file_name);

        normal_map(normal_map&&)            noexcept = default;
        normal_map& operator=(normal_map&&) noexcept = default;

        normal_map(const normal_map&)            = delete;
        normal_map& operator=(const normal_map&) = delete;

        /* Returns the normal in tangent space at the given UV-coordinates u, v (between 0 and 1) */
        /* Returns the local normal at the given UV-coordinates u, v (between 0 and 1) */
        inline const rt::vector& get_tangent_space_normal(const real u, const real v) const {
            const int x = u * width_real;
            const int y = v * height_real;
            // Due to floating-point imprecision, some "unit" vector have a norm slightly larger than 1,
            // producing out of range coordinates
            return data[ std::clamp(y, 0, height) ][ std::clamp(x, 0, width) ]; 
        }
};

template<>
inline constexpr std::string type_str<normal_map>() { return "normal map"; }