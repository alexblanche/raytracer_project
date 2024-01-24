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
        sphere(rt::vector c, double r, rt::color co);
        sphere();
        rt::vector get_center();
        double get_radius();
        rt::color get_color();
        double send(ray r);
        hit intersect(ray r);

        hit intersect2(ray r,double t);

}
;
#endif // RT_SPHERE_H
