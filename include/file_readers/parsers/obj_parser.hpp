#pragma once

#include "file_readers/parsers/scene_parser.hpp"

#include "auxiliary/exit_status.hpp"

#include <string>
#include <optional>

struct pre_parsing_info_obj {
    unsigned int faces      = 0;
    unsigned int triangles  = 0;
    unsigned int quads      = 0;
};

pre_parsing_info_obj pre_parse_obj(const std::string& filename);

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
*/
exit_status parse_obj_file(const std::string& file_name, const std::optional<unsigned int> default_texture_index,
   containers& containers,
   real scale, const rt::vector& shift,
   bool bounding_enabled, unsigned int polygons_per_bounding,
   const bounding*& output_bd, std::optional<real> gamma = std::nullopt);