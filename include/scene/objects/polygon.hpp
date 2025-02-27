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

        // virtual ~polygon() {}

        /* Returns the barycenter of the polygon */
        virtual rt::vector get_barycenter() const = 0;
        
        /* Returns the barycentric info (l1, l2) */
        virtual barycentric_info get_barycentric(const rt::vector& p) const = 0;

        /* Prints the polygon */
        virtual void print() const = 0;
};