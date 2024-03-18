#pragma once

#include "scene/objects/object.hpp"

#include <vector>

/* Wavefront .obj file parser */
/* Only handles .obj files made up of triangles and quads, for now.
   In the future, maybe split polygons with >= 5 sides into triangles */

/* Parses .obj file file_name. Triangles and quads are added to obj_set, and
   the  */
void parse_obj_file(const char* file_name, std::vector<const object*>& obj_set,
    std::vector<string>& material_names, std::vector<material>& material_set);