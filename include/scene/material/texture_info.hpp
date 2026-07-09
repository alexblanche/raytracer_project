#pragma once

#include "scene/material/barycentric.hpp"

#include "scene/material/texture.hpp"
#include "scene/material/normal_map.hpp"
// To do :
// #include "scene/material/roughness_map.hpp"
// #include "scene/material/displacement_map.hpp"

#include <array>
#include <optional>

/* Struct representing UV-coordinates */
struct uvcoord {
    real u, v;
};

class texture_info {

    public:
        static constexpr int NONE = -1;

        /* Vector of UV coordinates (between 0 and 1)
           6 for a triangle (u0,v0,u1,v1,u2,v2) and 8 for a quad */
        std::array<real, 8> uv_coordinates;

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
            //std::vector<real>&& uv_coordinates);
            //std::initializer_list<double>&& uv_coordinates
            std::array<real, 8>&& uv_coords); // parsing requires double type, then converted to real

        texture_info(texture_info&&) noexcept        = default;
        texture_info(const texture_info&)            = delete;
        texture_info& operator=(texture_info&&)      = delete;
        texture_info& operator=(const texture_info&) = delete;

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
