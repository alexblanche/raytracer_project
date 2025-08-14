#pragma once

#include <vector>
#include "screen/color.hpp"
#include "light/vector.hpp"

#include <iostream>


/* Class representing texture data

   All textures from a scene are stored in the static vector texture::set,
   and materials can store a texture_info object, pointing to a texture object
   and coordinates from this texture
 */

class texture {

    private:
        std::vector<std::vector<rt::color>> data;
        real width_minus_one, height_minus_one;
        int width, height;

    public:      

        /* Constructors */

        texture();

        /* Default constructor */
        texture(const int width, const int height, const std::vector<std::vector<rt::color>>&& data);

        /* Constructor from a .bmp or .hdr file
           Writes true in parsing_successful if the operation was successful */
        texture(const char* file_name, bool& parsing_successful, const real gamma = 1.0f);

        /* Accessor */

        /* Returns the color stored in data at UV-coordinates u, v (between 0 and 1) times width, height */
        const rt::color& get_color(const real u, const real v) const;

        ~texture() {
            // printf("Destruction of a texture (width = %d, height = %d, data = %lu)\n", width, height, data.size());
        }
        
        texture(const texture&) = delete;

        texture& operator=(const texture&) = delete;

        texture(texture&&) = default;

        texture& operator=(texture&&) = default;
};
