#pragma once

#include "object.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"
#include "auxiliary/randomgen.hpp"

#include <optional>

class sphere : public object {
    
    private:

        real radius;
        // Direction vectors, used to orient the texture
        std::optional<rt::vector> forward_dir;
        std::optional<rt::vector> right_dir;
        std::optional<rt::vector> up_dir;

    public:

        /* Constructors */
        
        sphere();

        sphere(const rt::vector& center, const real radius, const size_t material_index);

        sphere(const rt::vector& center, const real radius, const size_t material_index,
            const std::optional<texture_info>& info, const rt::vector& forward_dir, const rt::vector& right_dir);

        
        /* Intersection determination */

        std::optional<real> measure_distance(const ray& r) const;

        hit compute_intersection(ray& r, const real t) const;

        /* Minimum and maximum coordinates */
        min_max_coord get_min_max_coord() const;

        /* Returns the barycentric info for the object (l1 = longitude, l2 = latitude) (both between 0 and 1) */
        barycentric_info get_barycentric(const rt::vector& p) const;

        /* Normal map vector computation at render time
        Local normal may be the normal of the triangle (for flat shading) or the smoothed normal, and in this case the tangent space should be reorthonormalized */
        rt::vector compute_normal_from_map(const rt::vector& tangent_space_normal, const rt::vector& local_normal) const;

        
        /* Sampling */

        /* Uniformly samples a point on the sphere */
        rt::vector sample(randomgen& rg) const;

        /* Uniformly samples a point on the sphere that is visible from pt */
        rt::vector sample_visible(randomgen& rg, const rt::vector& pt) const;

};
