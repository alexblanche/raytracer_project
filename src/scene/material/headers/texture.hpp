#pragma once

#include <vector>
#include "../../../screen/headers/color.hpp"

/* Class representing texture data

   All textures from a scene are stored in the static vector texture::set,
   and materials can store a texture_info object, pointing to a texture object
   and coordinates from this texture
 */

class texture {

    private:
        int width, height;
        std::vector<std::vector<rt::color>> data;

    public:

        /* Texture set containing all the textures from a scene */
        static std::vector<const texture*> set;
        

        /* Constructors */

        /* Default constructor */
        texture(const int width, const int height, const std::vector<std::vector<rt::color>>& data);

        /* Constructor from a .bmp file */
        texture::texture(const char* file_name);


        /* Accessor */

        /* Returns the color stored in data at coordinates x, y between 0 and 1 times width, height */
        rt::color get_color(const double& x, const double& y) const;
};