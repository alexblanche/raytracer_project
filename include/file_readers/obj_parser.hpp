#pragma once

#include "scene/objects/object.hpp"

#include <vector>
#include <string>


/* Wavefront .obj file parser */
/* Only handles .obj files made up of triangles and quads, for now.
   In the future, maybe split polygons with >= 5 sides into triangles */

/* Parses .obj file file_name. Triangles and quads are added to obj_set,
   with materials indices found in material_names, and texture indices found
   in texture_names

   Object names (o), polygon groups (g), smooth shading (s), lines (l) are ignored

   Returns true if the operation was successful
 */
bool parse_obj_file(const char* file_name, std::vector<const object*>& obj_set,
   const unsigned int material_index, std::vector<string>& texture_names);