#pragma once

#include "legacy/objects/legacy_object.hpp"

class plane : public object {
    
    private:

        rt::vector normal;
        real d;

        /* A plane (P) of equation (P): ax + by + cz + d = 0
         defined by 4 reals a,b,c,d */

    public:

        plane();
        
        plane(real sa, real sb, real sc, real sd,
            const rt::color& col);
        
        plane(real pa, real pb, real pc, const rt::vector& position,
            const rt::color& col);


        /* Intersection determination */

        std::optional<real> measure_distance(const ray& r) const;
        
        hit compute_intersection(ray& r, real t) const;
};
