#include "scene/material/texture_info.hpp"

/** Texture infos **/

#include <iostream>

texture_info::texture_info()
    : uv_coordinates({}), texture_index(NONE) {}

texture_info::texture_info(
    std::optional<int> t_index,
    std::optional<int> n_index,
//  std::optional<int> roughness_map_index,
//  std::optional<int> displacement_map_index,
    std::array<real, 8>&& uv_coords)
    :   uv_coordinates(std::move(uv_coords)),
        texture_index   (t_index.value_or(NONE)),
        normal_map_index(n_index.value_or(NONE)) {}

/* Texturing */

/* Returns the UV-coordinate of the barycenter associated with the barycentric coordinates l1, l2
   In the case of quads, the boolean lower_triangle indicates that the three points to
   consider are (u0, v0), (u1, v1), (u2, v2) or (u0, v0), (u3, v3), (u2, v2) (in this order) */
uvcoord texture_info::get_barycenter(const barycentric_info& bary) const {
    
    uvcoord uvc;
    auto& [ u, v ] = uvc;
    const auto [ l1, l2 ] = bary.l;
    
    using enum object_type;
    switch (bary.type) {
        case Sphere:
        case Plane:
            u = l1;
            v = l2;
            break;
        case Quad:
            if (bary.triangle_side == side::LowerTriangle) {
                // Quads with (u0, v0), (u3, v3), (u2, v2) (in this order) considered
                const real l0 = 1.0_r - l1 - l2;
                u = l0 * uv_coordinates[0] + l1 * uv_coordinates[6] + l2 * uv_coordinates[4];
                v = l0 * uv_coordinates[1] + l1 * uv_coordinates[7] + l2 * uv_coordinates[5];
                break;
            }
            // else: same as Triangle case
            [[fallthrough]];
        case Triangle: {
                // Triangles or Quads with (u0, v0), (u1, v1), (u2, v2) considered
                const real l0 = 1.0_r - l1 - l2;
                u = l0 * uv_coordinates[0] + l1 * uv_coordinates[2] + l2 * uv_coordinates[4];
                v = l0 * uv_coordinates[1] + l1 * uv_coordinates[3] + l2 * uv_coordinates[5];
            }
            break;
        default:
            u = 0.0_r;
            v = 0.0_r;
            break;
    }
    return uvc;
}

/* Sets the tangent and bitangent vectors */
void texture_info::set_tangent_space(const rt::vector& t, const rt::vector& b) {
    tangent   = t;
    bitangent = b;
}