#pragma once

#ifndef RT_SPHERE_H
#define RT_SPHERE_H

#include "../../screen/headers/color.hpp"
#include "../../light/headers/vector.hpp"
#include "../../light/headers/hit.hpp"

class sphere
{
    private:
        rt::vector center;
        double radius;
        rt::color col;

    public:
        /* Constructors */
        sphere(const rt::vector& c, double r, const rt::color& co);

        sphere();

        /* Accessors */
        rt::vector get_center() const;

        double get_radius() const;

        rt::color get_color() const;

        
        /* Intersection determination */

        double send(const ray& r) const;

        //hit intersect(const ray& r) const;

        hit intersect(const ray& r, double t) const;
};

#endif // RT_SPHERE_H
