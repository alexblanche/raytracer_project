#pragma once

#include "object.hpp"

#include "../../screen/headers/color.hpp"
#include "../../light/headers/vector.hpp"
#include "../../light/headers/hit.hpp"

class plane : public object {
    
    private:

        double a, b, c, d;

        /* A plane (P) of equation (P): ax + by + cz + d = 0
         defined by 4 doubles a,b,c,d */

    public:

        /* Constructors */

        plane();
        
        plane(const double a, const double b, const double c, const double d,
            const rt::color& color,
            const unsigned int index);
        
        plane(const double a, const double b, const double c, const rt::vector& position,
            const rt::color& color,
            const unsigned int index);

        /* Accessors */

        rt::vector get_normal() const;

        double get_d() const;


        /* Intersection determination */

        double send(const ray& r) const;
        
        hit intersect(const ray& r, const double t) const;
};
