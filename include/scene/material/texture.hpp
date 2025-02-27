#pragma once

#include <vector>
#include "screen/color.hpp"
#include "scene/material/barycentric.hpp"

#include <iostream>

/* Struct representing UV-coordinates */
struct uvcoord {
    real u, v;
    uvcoord(const real u, const real v)
        : u(u), v(v) {}
};


/* Class representing texture data

   All textures from a scene are stored in the static vector texture::set,
   and materials can store a texture_info object, pointing to a texture object
   and coordinates from this texture
 */

class texture {

    private:
        int width, height;
        std::vector<std::vector<rt::color>> data;
        real width_minus_one, height_minus_one;

    public:      

        /* Constructors */

        texture();

        /* Default constructor */
        texture(const int width, const int height, const std::vector<std::vector<rt::color>>& data);

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

class texture_info {

    private:
        /* Texture index in texture::set */
        size_t texture_index;

        /* Vector of UV coordinates (between 0 and 1)
           6 for a triangle (u0,v0,u1,v1,u2,v2) and 8 for a quad */
        std::vector<real> uv_coordinates;

    public:
        texture_info();

        texture_info(size_t index, std::vector<real>&& uv_coordinates);

        /* Write in u, v the UV-coordinate of the barycenter associated with the barycentric coordinates l1, l2
        In the case of quads, the boolean lower_triangle indicates that the three points to
        consider are (u0, v0), (u1, v1), (u2, v2) or (u0, v0), (u3, v3), (u2, v2) (in this order) */
        uvcoord get_barycenter(const barycentric_info& bary) const;

        /* Returns the color of the pixel associated with UV-coordinates u, v */
        const rt::color& get_texture_color(const barycentric_info& bary,
            const std::vector<texture>& texture_set) const;
};