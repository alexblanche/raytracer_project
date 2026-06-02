#pragma once

#include "scene/objects/object.hpp"

#include "light/hit.hpp"
#include "scene/material/material.hpp"

struct plane_orientation {
    rt::vector right_dir;
    rt::vector down_dir;
    real inv_texture_scale = 1.0f;
};

class plane : public object {
    
    private:

        rt::vector normal;
        real d;
        // Used to orient the texture
        plane_orientation orientation;

        /* A plane (P) of equation (P): ax + by + cz + d = 0
         defined by 4 reals a,b,c,d, where normal = (a, b, c) */

    public:

        plane();
        
        plane(real sa, real sb, real sc, real sd,
            unsigned int material_index);
        
        plane(real a, real b, real c, const rt::vector& position,
            unsigned int material_index);

        plane(real a, real b, real c, const rt::vector& position,
            unsigned int material_index,
            unsigned int texture_info_index, const rt::vector& right, real scale);


        inline const rt::vector& get_normal() const {
            return normal;
        }

        /* Intersection determination */

        std::optional<real> measure_distance(const ray& r) const override;
        
        hit compute_intersection(ray& r, real t) const override;

        /* Returns the barycentric info (tiles according to texture_scale) */
        barycentric_info get_barycentric(const rt::vector& p) const override;

        /* Normal map vector computation at render time */
        rt::vector compute_normal_from_map(const rt::vector& tangent_space_normal, const rt::vector& local_normal, const texture_info& info) const override;

        /* Minimum and maximum coordinates (undefined for planes )*/
        min_max_coord get_min_max_coord() const override;

        rt::vector sample(randomgen& rg) const override;
        
        rt::vector sample_visible(randomgen& rg, const rt::vector& pt) const override;

};
