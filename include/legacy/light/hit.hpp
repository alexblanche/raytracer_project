#pragma once

#include "light/vector.hpp"
#include "light/ray.hpp"

#include <cmath>

#define TWOPI 6.2831853071795862

/** The hit class contains the information
 * of a ray hitting a surface: the ray in question,
 * the point of contact, the normal of the surface at
 * this point and the index of the object the surface belongs to.
*/

/* Forward-declaring the object class, to solve mutual recursivity between the hit and object classes */
class object;

class hit {
    private:
        rt::vector point;
        rt::vector normal;
        const object* hit_object;

    public:
        /* Main constructor */
        hit(const rt::vector& point, const rt::vector& normal, const object* hit_object);


        /* Default constructor */
        hit();

        /* Accessors */
        inline rt::vector get_point() const {
            return point;
        }
        
        inline rt::vector get_normal() const {
            return normal;
        }

        inline const object* get_object() const {
            return hit_object;
        }
};

