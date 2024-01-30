#pragma once

#include "../../screen/headers/color.hpp"
#include "../../light/headers/vector.hpp"
#include "../../light/headers/hit.hpp"


class plane {
    
    private:

        double a;
        double b;
        double c;
        double d;
        rt::color col;
        /* A plane (P) of equation (P): ax + by + cz + d = 0
         defined by 4 doubles a,b,c,d */

    public:

        /* Constructors */
        plane();
        
        plane(double ca, double cb, double cc, double cd, const rt::color& ccol);
        
        plane(double ca, double cb, double cc, const rt::vector& v, const rt::color& ccol);

        /* Accessors */
        rt::vector get_normal() const;

        double get_d() const;
        
        rt::color get_color() const;

        /* Intersection determination */

        double send(const ray& r) const;
        
        hit intersect(const ray& r, double t) const;
};
