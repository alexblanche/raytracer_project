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

class texture_info {

    private:
        /* Texture index in texture::set */
        unsigned int texture_index;

        /* Vector of UV coordinates (between 0 and 1)
           6 for a triangle (u0,v0,u1,v1,u2,v2) and 8 for a quad */
        std::vector<double> uv_coordinates;

    public:
        texture_info();

        texture_info(unsigned int index, std::vector<double> uv_coordinates);

        /* Write in u, v the UV-coordinate of the barycenter associated with the barycentric coordinates l1, l2
        In the case of quads, the boolean lower_triangle indicates that the three points to
        consider are (u0, v0), (u1, v1), (u2, v2) or (u0, v0), (u3, v3), (u2, v2) (in this order) */
        void get_barycenter(const double& l1, const double& l2, const bool lower_triangle,
            double& u, double& v) const;

        /* Returns the color of the pixel associated with UV-coordinates u, v */
        rt::color get_texture_color(const double& l1, const double& l2, const bool lower_triangle,
            const std::vector<texture>& texture_set) const;
};