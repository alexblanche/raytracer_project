#pragma once

#include "scene/objects/object.hpp"

struct plane_orientation {
    rt::vector right_dir   = rt::vector(1, 0,  0);
    rt::vector down_dir    = rt::vector(0, 0, -1);
    real inv_texture_scale = 1.0_r;
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

        plane(real pa, real pb, real pc, const rt::vector& position,
            unsigned int material_index);
        
        plane(real pa, real pb, real pc, real pd,
            unsigned int material_index);

        plane(real pa, real pb, real pc, const rt::vector& position,
            unsigned int material_index,
            unsigned int texture_info_index, const rt::vector& right, real scale);


        inline const rt::vector& get_normal() const {
            return normal;
        }

        /* Intersection determination */

        real measure_distance(const ray& r) const final;
        
        hit compute_intersection(const ray& r, real t) const final;

        /* Returns the barycentric info (tiles according to texture_scale) */
        barycentric_info get_barycentric(const rt::vector& p) const final;

        /* Normal map vector computation at render time */
        rt::vector compute_normal_from_map(const rt::vector& tangent_space_normal, const rt::vector& local_normal, const texture_info& info) const final;

        /* Minimum and maximum coordinates (undefined for planes )*/
        min_max_coord get_min_max_coord() const final;

        rt::vector sample(const randomgen& rg) const final;
        
        rt::vector sample_visible(const randomgen& rg, const rt::vector& pt) const final;

};
