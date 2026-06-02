#pragma once

#include "scene/objects/object.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"

class cylinder : public object {
    
    private:

        rt::vector direction;
        real radius, length;

    public:
        
        cylinder();

        cylinder(const rt::vector& origin, const rt::vector& direction,
            real radius, real length,
            unsigned int material_index);

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

        std::optional<real> measure_distance(const ray& r) const override;

        hit compute_intersection(ray& r, real t) const override;

        /* Minimum and maximum coordinates */
        min_max_coord get_min_max_coord() const override;

        /* Returns the barycentric info (the texture is mapped onto the top, the bottom, and the curved surface) */
        barycentric_info get_barycentric(const rt::vector& p) const override;

        rt::vector compute_normal_from_map(
            const rt::vector& tangent_space_normal,
            const rt::vector& local_normal,
            const texture_info& info
        ) const override;

        rt::vector sample(randomgen& rg) const override;
        
        rt::vector sample_visible(randomgen& rg, const rt::vector& pt) const override;
};
