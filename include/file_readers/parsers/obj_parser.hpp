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
   The polygons with >= 5 sides are split into triangles */

/* Positioning information for 3D models */
class model_positioning {

    private:
        // TODO: mat3<Col> rotation;
        rt::vector shift = rt::ZERO;
        real scale       = 1.0_r;
        static_assert(TODO_ROTATION_OF_MODELS);

        // bool rotation_is_set = false;
        bool shift_is_set    = false;
        bool scale_is_set    = false;
    
    public:

        model_positioning() {}

        model_positioning(const std::optional<rt::vector> shift,
            const std::optional<real> scale = std::nullopt)
        
            :   shift(shift.value_or(rt::ZERO)), scale(scale.value_or(1.0_r)),
                shift_is_set(shift.has_value()), scale_is_set(scale.has_value()) {}

        bool is_not_null() const {
            return shift_is_set || scale_is_set;
        }

        std::tuple<rt::vector, real> get_content() const {
            return std::make_tuple(shift, scale);
        }

        rt::vector position(const rt::vector& v) const {
            return fma(v, scale, shift);
        }
};

/* Parses .obj file file_name. Triangles and quads are added to obj_set,
    with material indices (defined with the keyword usemtl) found in material_names
   
    - Only one texture is handled.
    - Object names (o), polygon groups (g), smooth shading (s), lines (l) are ignored.
    - The object is scaled with the factor scale, and shifted by the vector shift.
    - If bounding_enabled, a bounding containing the whole object is placed in output_bd.
        It contains a hierarchy of bounding boxes, such that the terminal ones contain at most
        polygons_per_bounding polygons.
*/
exit_status parse_obj_file(const std::string& file_name, std::optional<unsigned int> default_texture_index,
    containers& containers,
    const model_positioning& positioning,
    std::optional<bvh>& bvh, std::optional<real> gamma = std::nullopt);