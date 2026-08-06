#pragma once

#include "scene/objects/object.hpp"
#include "math/geometry/mat3.hpp"
#include "scene/material/mapping_info.hpp"

class sphere final : public object {
    
    private:

        real radius;
        real radius_sq; // pre-computation
        
        // Direction vectors, used to orient the texture
        // sphere_orientation orientation; -> moved to its own structure (sphere::orientation)

    public:

        struct orientation : public mapping_info {
            
            // Rows: right_dir, up_dir, forward_dir
            linalg::mat3<linalg::mat_type::Row> matrix;

            orientation(int index, composition comp,
                const rt::vector& forward_dir, const rt::vector& right_dir);
        };


        sphere(const rt::vector& center, real radius, unsigned int material_index,
            unsigned int orientation_info_index = EMPTY_INDEX);

        // sphere(const rt::vector& center, real radius, unsigned int material_index,
        //     unsigned int texture_info_index, const rt::vector& forward_dir, const rt::vector& right_dir);

        sphere(sphere&&) noexcept        = default;
        sphere(const sphere&)            = delete;
        sphere& operator=(const sphere&) = delete;
        sphere& operator=(sphere&&)      = delete;
        
        /* Intersection determination */

        real measure_distance(const ray& r) const override;

        hit compute_intersection(const ray& r, real t) const override;

        /* Minimum and maximum coordinates */
        min_max_coord get_min_max_coord() const override;

        // /* Returns the barycentric info for the object (l1 = longitude, l2 = latitude) (both between 0 and 1) */
        // barycentric_info get_barycentric(const rt::vector& p) const override;

        uvcoord compute_uv(const rt::vector& p, const mapping_info* orientation_info) const override;

        rt::vector compute_normal_from_map(
            const rt::vector& tangent_space_normal,
            const rt::vector& local_normal,
            const mapping_info* orientation_info
        ) const override;

        /* Uniformly samples a point on the sphere */
        rt::vector sample(const randomgen& rg) const override;

        /* Uniformly samples a point on the sphere that is visible from pt */
        rt::vector sample_visible(const randomgen& rg, const rt::vector& pt) const override;

        void print() const override;
};
