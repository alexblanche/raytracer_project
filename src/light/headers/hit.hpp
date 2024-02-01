#pragma once

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
        unsigned int obj_index;

    public:
        /* Main constructor */
        hit(const ray& g, const rt::vector& p, const rt::vector& n, const unsigned int i);

        hit();

        /* Accessors */
        ray get_ray() const;

        rt::vector get_point() const;
        
        rt::vector get_normal() const;

        unsigned int get_obj_index() const;
};