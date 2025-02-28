#pragma once

#include "object.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"

#include <optional>

class plane : public object {
    
    private:

        rt::vector normal;
        real a, b, c, d;
        // Used to orient the texture
        std::optional<rt::vector> right_dir;
        std::optional<rt::vector> down_dir;
        real inv_texture_scale;

        /* A plane (P) of equation (P): ax + by + cz + d = 0
         defined by 4 reals a,b,c,d */

    public:

        /* Constructors */

        plane();
        
        plane(const real sa, const real sb, const real sc, const real sd,
            const unsigned int material_index);
        
        plane(const real a, const real b, const real c, const rt::vector& position,
            const unsigned int material_index);

        plane(const real a, const real b, const real c, const rt::vector& position,
            const unsigned int material_index,
            const std::optional<texture_info>& info, const rt::vector& right, const real scale);

        /* Accessors */

        inline const rt::vector& get_normal() const {
            return normal;
        }


        /* Intersection determination */

        std::optional<real> measure_distance(const ray& r) const;
        
        hit compute_intersection(ray& r, const real t) const;

        /* Returns the barycentric info (tiles according to texture_scale) */
        barycentric_info get_barycentric(const rt::vector& p) const;
};
