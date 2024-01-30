#pragma once

#ifndef PLANE_H
#define PLANE_H

#include "../../screen/headers/color.hpp"
#include "../../light/headers/vector.hpp"
#include "../../light/headers/hit.hpp"


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
        plane(double ca, double cb, double cc, double cd, const rt::color& ccol);
        plane(double ca, double cb, double cc, const rt::vector& v, const rt::color& ccol);

        rt::vector get_normal() const;
        double get_d() const;
        rt::color get_color() const;

        double send(const ray& r) const;
        hit intersect(const ray& r, double t) const;
};

#endif // PLANE_H
