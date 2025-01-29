#pragma once

#include "object.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"

#include <optional>

class plane : public object {
    
    private:

        real a, b, c, d;

        /* A plane (P) of equation (P): ax + by + cz + d = 0
         defined by 4 reals a,b,c,d */

    public:

        /* Constructors */

        plane();
        
        plane(const real sa, const real sb, const real sc, const real sd,
            const unsigned int material_index);
        
        plane(const real a, const real b, const real c, const rt::vector& position,
            const unsigned int material_index);

        /* Accessors */

        inline rt::vector get_normal() const {
            return rt::vector(a, b, c);
        }


        /* Intersection determination */

        std::optional<real> measure_distance(const ray& r) const;
        
        hit compute_intersection(ray& r, const real t) const;
};
