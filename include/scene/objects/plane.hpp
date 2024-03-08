#pragma once

#include "object.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"

class plane : public object {
    
    private:

        double a, b, c, d;

        /* A plane (P) of equation (P): ax + by + cz + d = 0
         defined by 4 doubles a,b,c,d */

    public:

        /* Constructors */

        plane();
        
        plane(const double sa, const double sb, const double sc, const double sd,
            const material& material);
        
        plane(const double a, const double b, const double c, const rt::vector& position,
            const material& material);

        /* Accessors */

        inline rt::vector get_normal() const {
            return rt::vector(a, b, c);
        }


        /* Intersection determination */

        double measure_distance(const ray& r) const;
        
        hit compute_intersection(const ray& r, const double t) const;
};
