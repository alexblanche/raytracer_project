#pragma once

#include "scene/objects/object.hpp"

class cylinder : public object {
    
    private:

        rt::vector direction;
        real radius, length;

    public:

        cylinder(const rt::vector& origin, const rt::vector& direction,
            real radius, real length,
            unsigned int material_index);

        cylinder(cylinder&&) noexcept        = default;
        cylinder(const cylinder&)            = delete;
        cylinder& operator=(const cylinder&) = delete;
        cylinder& operator=(cylinder&&)      = delete;

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

        real measure_distance(const ray& r) const final;

        hit compute_intersection(const ray& r, real t) const final;

        /* Minimum and maximum coordinates */
        min_max_coord get_min_max_coord() const final;

        /* Returns the barycentric info (the texture is mapped onto the top, the bottom, and the curved surface) */
        barycentric_info get_barycentric(const rt::vector& p) const final;

        rt::vector compute_normal_from_map(
            const rt::vector& tangent_space_normal,
            const rt::vector& local_normal,
            const texture_info& info
        ) const final;

        rt::vector sample(const randomgen& rg) const final;
        
        rt::vector sample_visible(const randomgen& rg, const rt::vector& pt) const final;
};
