#include "scene/material/texture_info.hpp"

/** Texture infos **/

texture_info::texture_info()
    : uv_coordinates({}), texture_index(NONE) {}

texture_info::texture_info(std::optional<int> t_index,
    std::optional<int> n_index,
    // std::optional<size_t> roughness_map_index,
    // std::optional<size_t> displacement_map_index,
    std::vector<real>&& uv_coordinates)
    : uv_coordinates(std::move(uv_coordinates)),
    texture_index(t_index.value_or(NONE)),
    normal_map_index(n_index.value_or(NONE)) {}

/* Texturing */

/* Returns the UV-coordinate of the barycenter associated with the barycentric coordinates l1, l2
   In the case of quads, the boolean lower_triangle indicates that the three points to
   consider are (u0, v0), (u1, v1), (u2, v2) or (u0, v0), (u3, v3), (u2, v2) (in this order) */
uvcoord texture_info::get_barycenter(const barycentric_info& bary) const {
    
    real u, v;
    switch (bary.type) {
        case object_type::Sphere:
        case object_type::Plane:
            u = bary.l1;
            v = bary.l2;
            break;
        case object_type::Quad:
            if (bary.triangle_side == side::LowerTriangle) {
                // Quads with (u0, v0), (u3, v3), (u2, v2) (in this order) considered
                const real l0 = 1.0f - bary.l1 - bary.l2;
                u = l0 * uv_coordinates[0] + bary.l1 * uv_coordinates[6] + bary.l2 * uv_coordinates[4];
                v = l0 * uv_coordinates[1] + bary.l1 * uv_coordinates[7] + bary.l2 * uv_coordinates[5];
                break;
            }
            // else: same as Triangle case
        case object_type::Triangle: {
                // Triangles or Quads with (u0, v0), (u1, v1), (u2, v2) considered
                const real l0 = 1.0f - bary.l1 - bary.l2;
                u = l0 * uv_coordinates[0] + bary.l1 * uv_coordinates[2] + bary.l2 * uv_coordinates[4];
                v = l0 * uv_coordinates[1] + bary.l1 * uv_coordinates[3] + bary.l2 * uv_coordinates[5];
            }
            break;
        default:
            u = 0;
            v = 0;
            break;
    }
    return { u, v };
}

/* Sets the tangent and bitangent vectors */
void texture_info::set_tangent_space(const rt::vector& t, const rt::vector& b) {
    tangent   = t;
    bitangent = b;
}