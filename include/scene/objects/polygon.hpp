#pragma once

#include "object.hpp"
#include "scene/material/texture.hpp"
#include "scene/material/barycentric.hpp"


/* Virtual class derived from object, containing triangles and quads */

class polygon : public object {

    public:
        /* Texture info (empty if the polygon is not textured) */
        texture_info info;

        polygon() : object(), info(texture_info()) {}

        /* Constructor for non-textured polygons */
        polygon(const rt::vector& position, const unsigned int material_index)
            : object(position, material_index), info(texture_info()) {}

        /* Constructor for textured polygons */
        polygon(const rt::vector& position, const unsigned int material_index,
            const texture_info& info)
            
            : object(position, material_index, true), info(info) {}
        
        /* Writes the barycentric coordinates in variables l1, l2
           The boolean return value is used for determining the three points considered in quads */
        virtual barycentric_info get_barycentric(const rt::vector& p) const = 0;
};