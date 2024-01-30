#pragma once

#include "object.hpp"

#include "../../screen/headers/color.hpp"
#include "../../light/headers/vector.hpp"
#include "../../light/headers/hit.hpp"

class plane : public object {
    
    private:

        double a;
        double b;
        double c;
        double d;

        /* A plane (P) of equation (P): ax + by + cz + d = 0
         defined by 4 doubles a,b,c,d */

    public:

        /* Constructors */

        plane();
        
        plane(double a, double b, double c, double d, const rt::color& color);
        
        plane(double a, double b, double c, const rt::vector& position, const rt::color& color);

        /* Accessors */

        rt::vector get_normal() const;

        double get_d() const;


        /* Intersection determination */

        double send(const ray& r) const;
        
        hit intersect(const ray& r, double t) const;
};
