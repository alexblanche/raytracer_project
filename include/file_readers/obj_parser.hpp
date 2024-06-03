#pragma once

#include "scene/objects/object.hpp"
#include "scene/objects/bounding.hpp"
#include "scene/material/texture.hpp"

#include <vector>
#include <string>

#include "file_readers/parsing_wrappers.hpp"

#include <optional>

/* Wavefront .obj file parser */
/* Only handles .obj files made up of triangles and quads, for now.
   In the future, maybe split polygons with >= 5 sides into triangles */

/* Parses .obj file file_name. Triangles and quads are added to obj_set,
   with material indices (defined with the keyword usemtl) found in material_names
   
   - Only one texture is handled.
   - Object names (o), polygon groups (g), smooth shading (s), lines (l) are ignored.
   - The object is scaled with the factor scale, and shifted by the vector shift.
   - If bounding_enabled, a bounding containing the whole object is placed in output_bd.
     It contains a hierarchy of bounding boxes, such that the terminal ones contain at most
     polygons_per_bounding polygons.
   
   Returns true if the operation was successful
*/
bool parse_obj_file(const char* file_name, const std::optional<size_t> default_texture_index,
   std::vector<const object*>& obj_set,
   std::vector<wrapper<material>>& material_wrapper_set,
   std::vector<wrapper<texture>>& texture_wrapper_set,
   const double& scale, const rt::vector& shift,
   const bool bounding_enabled, const unsigned int polygons_per_bounding,
   const bounding*& output_bd);