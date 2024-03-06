#pragma once

#include <vector>
#include "screen/color.hpp"

/* Class representing texture data

   All textures from a scene are stored in the static vector texture::set,
   and materials can store a texture_info object, pointing to a texture object
   and coordinates from this texture
 */

class texture {

    private:
        int width, height;
        std::vector<std::vector<rt::color>> data;
        double width_minus_one, height_minus_one;

    public:      

        /* Constructors */

        /* Default constructor */
        texture(const int width, const int height, const std::vector<std::vector<rt::color>>& data);

        /* Constructor from a .bmp file
           Writes true in parsing_successful if the operation was successful */
        texture(const char* file_name, bool& parsing_successful);


        /* Accessor */

        /* Returns the color stored in data at UV-coordinates u, v between 0 and 1 times width, height */
        rt::color get_color(const double& u, const double& v) const;
};
