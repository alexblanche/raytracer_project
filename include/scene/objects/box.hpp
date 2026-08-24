#pragma once

#include "scene/objects/object.hpp"
#include "scene/material/mapping_info.hpp"
#include "math/geometry/mat3.hpp"

class box final : public object {
    
    private:

        linalg::mat3<linalg::mat_type::Row> axes;
        rt::vector dims;

    public:

        class orientation final : public mapping_info {}; // Unused

        /* Main constructor */
        constexpr box(const rt::vector& center, const rt::vector& n1, const rt::vector& n2,
            real l1, real l2, real l3, unsigned int material_index = EMPTY_INDEX,
            unsigned int orientation_info_index = EMPTY_INDEX)

            :   object(center, material_index, orientation_info_index),
                axes(n1, n2, n1 ^ n2),
                dims(l1 / 2, l2 / 2, l3 / 2) {}

        box(const min_max_coord& mmc)
            :   object(rt::ZERO, EMPTY_INDEX),
                axes(rt::RIGHT, rt::UP, rt::FORWARD) {
            
            const auto& [ min_x, max_x, min_y, max_y, min_z, max_z ] = mmc;
            const rt::vector min(min_x, min_y, min_z);
            const rt::vector max(max_x, max_y, max_z);
        
            position = (max + min) / 2;
            dims     = (max - min) / 2;
        }

        box(box&&) noexcept        = default;
        box(const box&)            = delete;
        box& operator=(const box&) = delete;
        box& operator=(box&&)      = delete;

        static inline box infinite_box() {
            return box(rt::ZERO, rt::RIGHT, rt::UP, infinity, infinity, infinity, EMPTY_INDEX);
        }

        /* Intersection determination */

        real measure_distance(const ray& r) const override;
        
        hit compute_intersection(const ray& r, real t) const override;

        /* Minimum and maximum coordinates */
        min_max_coord get_min_max_coord() const override;

        /* Specific to (standard) boxes: returns true if the ray r hits the box
        The box is assumed to be standard (axes are n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1)) */
        bool is_hit_by(const ray& r) const;

        // Same as measure_distance but for AABB
        real is_hit_with_distance(const ray& r) const;

        /* Returns the barycentric info (the faces behave like quads) */
        // barycentric_info get_barycentric(const rt::vector& p) const override;

        uvcoord compute_uv(const rt::vector& p, const mapping_info* orientation_info) const override;

        rt::vector compute_normal_from_map(
            const rt::vector& tangent_space_normal,
            const rt::vector& local_normal,
            const mapping_info* orientation_info
        ) const override;

        rt::vector sample(const randomgen& rg) const override;
        
        rt::vector sample_visible(const randomgen& rg, const rt::vector& pt) const override;

        void print() const override;
};
