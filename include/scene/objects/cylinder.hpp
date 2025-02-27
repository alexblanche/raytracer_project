#pragma once

#include "object.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"

#include <optional>

class cylinder : public object {
    
    private:

        rt::vector direction;
        real radius, length;

    public:

        /* Constructors */
        
        cylinder();

        cylinder(const rt::vector& origin, const rt::vector& direction,
            const real radius, const real length,
            const size_t material_index);

        /* Accessors */

        inline rt::vector get_origin() const {
            return position;
        }

        inline real get_radius() const {
            return radius;
        }

        inline real get_length() const {
            return length;
        }
        
        /* Intersection determination */

        std::optional<real> measure_distance(const ray& r) const;

        hit compute_intersection(ray& r, const real t) const;

        /* Minimum and maximum coordinates */
        min_max_coord get_min_max_coord() const;

        /* Returns the barycentric info (the texture is mapped onto the top, the bottom, and the curved surface) */
        barycentric_info get_barycentric(const rt::vector& p) const;
};
