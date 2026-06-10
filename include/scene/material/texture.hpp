#pragma once

#include "light/vector.hpp"
#include "image/matrix.hpp"

#include <algorithm>
#include <string>


/* Class representing texture data

   All textures from a scene are stored in the static vector texture::set,
   and materials can store a texture_info object, pointing to a texture object
   and coordinates from this texture
 */

class texture {

    private:
        matrix data;
        int width, height;
        real width_real, height_real;

    public:      
        texture() {}

        texture(matrix&& matrix) :
            data(std::move(matrix)),
            width(data.width - 1), height(data.height - 1),
            width_real(width),
            height_real(height) {}

        /* Constructor from a .bmp or .hdr file
           Writes true in parsing_successful if the operation was successful */
        texture(const std::string& file_name, bool& parsing_successful, real gamma = 1.0_r);

        /* Returns the color stored in data at UV-coordinates u, v (between 0 and 1) times width, height */
        /* Returns the color stored in data at UV-coordinates u, v (between 0 and 1) times width, height */
        inline const rt::color& get_color(const real u, const real v) const {
            const int x = u * width_real;
            const int y = v * height_real;
            // Due to floating-point imprecision, some "unit" vector have a norm slightly larger than 1,
            // producing out of range coordinates
            return data[ std::clamp(y, 0, height), std::clamp(x, 0, width) ];
        }

        ~texture() = default;

        texture(texture&&)                  = default;
        texture& operator=(texture&&)       = default;

        texture(const texture&)             = delete;
        texture& operator=(const texture&)  = delete;
};
