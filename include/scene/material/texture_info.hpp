#pragma once

#include <vector>
#include "light/vector.hpp"
#include "scene/material/barycentric.hpp"

#include "scene/material/texture.hpp"
#include "scene/material/normal_map.hpp"
// To do :
// #include "scene/material/roughness_map.hpp"
// #include "scene/material/displacement_map.hpp"

/* Struct representing UV-coordinates */
struct uvcoord {
    real u, v;
    uvcoord(const real u, const real v)
        : u(u), v(v) {}
};

class texture_info {

    public:
        /* Index in the scene's sets */
        size_t texture_index;
        size_t normal_map_index;
        //size_t roughness_map_index;
        //size_t displacement_map_index;

        /* Vector of UV coordinates (between 0 and 1)
           6 for a triangle (u0,v0,u1,v1,u2,v2) and 8 for a quad */
        std::vector<real> uv_coordinates;

        /* Tangent and bitangent for normal mapping */
        rt::vector tangent;
        rt::vector bitangent;

        texture_info();

        texture_info(size_t index, std::vector<real>&& uv_coordinates);

        /* Sets the tangent and bitangent vectors */
        void set_tangent_space(const rt::vector& t, const rt::vector& b);

        /* Returns the UV-coordinate of the barycenter associated with the barycentric coordinates l1, l2
        In the case of quads, the boolean lower_triangle indicates that the three points to
        consider are (u0, v0), (u1, v1), (u2, v2) or (u0, v0), (u3, v3), (u2, v2) (in this order) */
        uvcoord get_barycenter(const barycentric_info& bary) const;
};
