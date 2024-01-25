#pragma once

#ifndef RT_HIT_H
#define RT_HIT_H

#include "../../screen/headers/color.hpp"
#include "vector.hpp"
#include "ray.hpp"

class hit {
    private:
        ray gen;
        rt::vector point;
        rt::vector normal;
        rt::color col;

    public:
        hit(ray g, rt::vector p, rt::vector n, rt::color c);
        hit();
        ray get_ray();
        rt::vector get_point();
        rt::vector get_normal();
        rt::color get_color();
};

#endif // RT_HIT_H