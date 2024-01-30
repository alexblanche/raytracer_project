#pragma once

#ifndef RT_HIT_H
#define RT_HIT_H

#include "../../screen/headers/color.hpp"
#include "vector.hpp"
#include "ray.hpp"

/** The hit class contains the information
 * of a ray hitting a surface: the ray in question,
 * the point of contact, the normal of the surface at
 * this point and the color of the surface.
*/

class hit {
    private:
        ray gen;
        rt::vector point;
        rt::vector normal;
        rt::color col;

    public:
        /* Main constructor */
        hit(const ray& g, const rt::vector& p, const rt::vector& n, const rt::color& c);

        hit();

        /* Accessors */
        ray get_ray() const;

        rt::vector get_point() const;
        
        rt::vector get_normal() const;
        
        rt::color get_color() const;
};

#endif // RT_HIT_H