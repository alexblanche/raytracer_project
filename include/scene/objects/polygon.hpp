#pragma once

#include "object.hpp"
#include "scene/material/texture.hpp"
#include "scene/material/barycentric.hpp"

#include <optional>


/* Virtual class derived from object, containing triangles and quads */

class polygon : public object {

    public:
        /* Texture info (empty if the polygon is not textured) */
        std::optional<texture_info> info;

        polygon() : object(), info(std::nullopt) {}

        /* For untextured polygons, provided nullopt for info */
        polygon(const rt::vector& position, const size_t material_index,
            const std::optional<texture_info>& info)
            
            : object(position, material_index, info.has_value()), info(info) {}
        
        /* Writes the barycentric coordinates in variables l1, l2
           The boolean return value is used for determining the three points considered in quads */
        virtual barycentric_info get_barycentric(const rt::vector& p) const = 0;
};