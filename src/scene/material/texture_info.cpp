#include "scene/material/texture_info.hpp"

/** Texture infos **/

texture_info::texture_info()
    : texture_index((size_t) -1), uv_coordinates({}) {}

texture_info::texture_info(size_t texture_index, std::vector<real>&& uv_coordinates)
    : texture_index(texture_index), uv_coordinates(std::move(uv_coordinates)) {}

/* Texturing */

/* Returns the UV-coordinate of the barycenter associated with the barycentric coordinates l1, l2
   In the case of quads, the boolean lower_triangle indicates that the three points to
   consider are (u0, v0), (u1, v1), (u2, v2) or (u0, v0), (u3, v3), (u2, v2) (in this order) */
uvcoord texture_info::get_barycenter(const barycentric_info& bary) const {

    const unsigned int n = uv_coordinates.size();
    if (n == 0) {
        // Spheres or Planes
        return uvcoord(bary.l1, bary.l2);
    }
    else if (n == 6 || bary.lower_triangle) {
        // Triangles or Quads with (u0, v0), (u1, v1), (u2, v2) considered
        const real u = (1.0f - bary.l1 - bary.l2) * uv_coordinates[0] + bary.l1 * uv_coordinates[2] + bary.l2 * uv_coordinates[4];
        const real v = (1.0f - bary.l1 - bary.l2) * uv_coordinates[1] + bary.l1 * uv_coordinates[3] + bary.l2 * uv_coordinates[5];
        return uvcoord(u, v);
    }
    else {
        // Quads with (u0, v0), (u3, v3), (u2, v2) (in this order) considered
        const real u = (1.0f - bary.l1 - bary.l2) * uv_coordinates[0] + bary.l1 * uv_coordinates[6] + bary.l2 * uv_coordinates[4];
        const real v = (1.0f - bary.l1 - bary.l2) * uv_coordinates[1] + bary.l1 * uv_coordinates[7] + bary.l2 * uv_coordinates[5];
        return uvcoord(u, v);
    }
}

/* Sets the tangent and bitangent vectors */
void texture_info::set_tangent_space(const rt::vector& t, const rt::vector& b) {
    tangent   = t;
    bitangent = b;
}