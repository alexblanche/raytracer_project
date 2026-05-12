#pragma once

#include "object.hpp"

#include "light/vector.hpp"
#include "legacy/light/hit.hpp"

class plane : public object {
    
    private:

        real a, b, c, d;

        /* A plane (P) of equation (P): ax + by + cz + d = 0
         defined by 4 reals a,b,c,d */

    public:

        /* Constructors */

        plane();
        
        plane(real sa, real sb, real sc, real sd,
            const rt::color& col);
        
        plane(real pa, real pb, real pc, const rt::vector& position,
            const rt::color& col);

        /* Accessors */

        inline rt::vector get_normal() const {
            return rt::vector(a, b, c);
        }


        /* Intersection determination */

        std::optional<real> measure_distance(const ray& r) const;
        
        hit compute_intersection(ray& r, real t) const;
};
