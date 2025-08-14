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
        /* Vector of UV coordinates (between 0 and 1)
           6 for a triangle (u0,v0,u1,v1,u2,v2) and 8 for a quad */
        std::vector<real> uv_coordinates;

        /* Tangent and bitangent for normal mapping */
        rt::vector tangent;
        rt::vector bitangent;

        /* Index in the scene's sets */
        // -1 = none
        int texture_index;
        int normal_map_index;
        // int roughness_map_index;
        // int displacement_map_index;

        texture_info();

        texture_info(std::optional<int> t_index,
            std::optional<int> n_index,
            std::vector<real>&& uv_coordinates);

        /* Sets the tangent and bitangent vectors */
        void set_tangent_space(const rt::vector& t, const rt::vector& b);

        /* Returns the UV-coordinate of the barycenter associated with the barycentric coordinates l1, l2
        In the case of quads, the boolean lower_triangle indicates that the three points to
        consider are (u0, v0), (u1, v1), (u2, v2) or (u0, v0), (u3, v3), (u2, v2) (in this order) */
        uvcoord get_barycenter(const barycentric_info& bary) const;

        inline bool has_texture_information() const {
            return texture_index >= 0;
        }

        inline bool has_normal_information() const {
            return normal_map_index >= 0;
        }
};
