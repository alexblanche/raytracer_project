#pragma once

#ifndef PLANE_H
#define PLANE_H

#include "../../screen/headers/color.hpp"
#include "../../screen/headers/vector.hpp"
//#include "hit.hpp"


class plane
{
    private:
        double a;
        double b;
        double c;
        double d;
        rt::color col;
        // Plane defined by a,b,c,d if its equation is
        // (P): ax + by + cz + d = 0
    public:
        plane();
        plane(double ca, double cb, double cc, double cd, rt::color ccol);
        plane(double ca, double cb, double cc, rt::vector v, rt::color ccol);

        rt::vector get_normal();
        double get_d();
        rt::color get_color();

        double send(ray r);
        //hit intersect(ray r, double t);
};

#endif // PLANE_H
