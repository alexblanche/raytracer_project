// #include "scene/material/texture_info.hpp"

// /** Texture infos **/

// texture_info::texture_info(
//     std::optional<int> t_index,
//     std::optional<int> n_index,
// //  std::optional<int> roughness_map_index,
// //  std::optional<int> displacement_map_index,
//     std::array<real, 8>&& uv_coords)
//     :   uv_coordinates(std::move(uv_coords)),
//         texture_index   (t_index.value_or(NONE)),
//         normal_map_index(n_index.value_or(NONE)) {}

// /* Texturing */

// /* Returns the UV-coordinate of the barycenter associated with the barycentric coordinates l1, l2
//    In the case of quads, the boolean lower_triangle indicates that the three points to
//    consider are (u0, v0), (u1, v1), (u2, v2) or (u0, v0), (u3, v3), (u2, v2) (in this order) */
// uvcoord texture_info::get_barycenter(const barycentric_info& bary) const {
    
//     uvcoord uvc;
//     auto& [ u, v ] = uvc;
//     const auto [ l1, l2 ] = bary.l;
    
//     using enum object_type;
//     switch (bary.type) {
//         case Sphere:
//         case Plane:
//             u = l1;
//             v = l2;
//             break;
//         case Quad:
//             if (bary.triangle_side == side::Triangle032) {
//                 // Quads with (u0, v0), (u3, v3), (u2, v2) (in this order) considered
//                 const auto& [ u0, v0, _, _, u2, v2, u3, v3 ] = uv_coordinates;
//                 const real l0 = 1.0_r - l1 - l2;
//                 u = l0 * u0 + l1 * u3 + l2 * u2;
//                 v = l0 * v0 + l1 * v3 + l2 * v2;
//                 break;
//             }
//             // else: same as Triangle case
//             [[fallthrough]];
//         case Triangle: {
//                 // Triangles or Quads with (u0, v0), (u1, v1), (u2, v2) considered
//                 const auto& [ u0, v0, u1, v1, u2, v2, _, _ ] = uv_coordinates;
//                 const real l0 = 1.0_r - l1 - l2;
//                 u = l0 * u0 + l1 * u1 + l2 * u2;
//                 v = l0 * v0 + l1 * v1 + l2 * v2;
//             }
//             break;
//         default:
//             u = 0.0_r;
//             v = 0.0_r;
//             break;
//     }
//     return uvc;
// }