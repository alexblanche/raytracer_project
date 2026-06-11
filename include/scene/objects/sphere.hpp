#pragma once

#include "scene/objects/object.hpp"

struct sphere_orientation {
    rt::vector forward_dir;
    rt::vector right_dir;
    rt::vector up_dir;
};

class sphere : public object {
    
    private:

        real radius;
        real radius_sq; // pre-computation
        // Direction vectors, used to orient the texture
        sphere_orientation orientation;

    public:

        sphere(const rt::vector& center, const real radius, unsigned int material_index);

        sphere(const rt::vector& center, const real radius, unsigned int material_index,
            unsigned int texture_info_index, const rt::vector& forward_dir, const rt::vector& right_dir);

        
        /* Intersection determination */

        real measure_distance(const ray& r) const override final;

        hit compute_intersection(const ray& r, real t) const override final;

        /* Minimum and maximum coordinates */
        min_max_coord get_min_max_coord() const override final;

        /* Returns the barycentric info for the object (l1 = longitude, l2 = latitude) (both between 0 and 1) */
        barycentric_info get_barycentric(const rt::vector& p) const override final;

        /* Normal map vector computation at render time
        Local normal may be the normal of the triangle (for flat shading) or the smoothed normal, and in this case the tangent space should be reorthonormalized */
        rt::vector compute_normal_from_map(const rt::vector& tangent_space_normal, const rt::vector& local_normal, const texture_info& info) const override final;

        
        /* Sampling */

        /* Uniformly samples a point on the sphere */
        rt::vector sample(randomgen& rg) const override final;

        /* Uniformly samples a point on the sphere that is visible from pt */
        rt::vector sample_visible(randomgen& rg, const rt::vector& pt) const override final;

};
