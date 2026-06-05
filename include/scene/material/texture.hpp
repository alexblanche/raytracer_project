#pragma once

#include "light/vector.hpp"
#include "image/matrix.hpp"

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
        real width_minus_one, height_minus_one;

    public:      
        texture() {}

        /* Default constructor */
        texture(matrix&& matrix) :
            data(std::move(matrix)),
            width(data.width), height(data.height),
            width_minus_one(data.width - 1),
            height_minus_one(data.height - 1) {}

        /* Constructor from a .bmp or .hdr file
           Writes true in parsing_successful if the operation was successful */
        texture(const std::string& file_name, bool& parsing_successful, real gamma = 1.0_r);

        /* Returns the color stored in data at UV-coordinates u, v (between 0 and 1) times width, height */
        const rt::color& get_color(real u, real v) const;

        ~texture() = default;

        texture(texture&&)                  = default;
        texture& operator=(texture&&)       = default;

        texture(const texture&)             = delete;
        texture& operator=(const texture&)  = delete;
};
