#pragma once

#include "vector.hpp"
#include "ray.hpp"
#include "auxiliary/randomgen.hpp"

#include <vector>

/** The hit class contains the information
 * of a ray hitting a surface: the ray in question,
 * the point of contact, the normal of the surface at
 * this point and a pointer to the object hit.
*/

/* Forward-declaring the object class, to solve mutual recursivity between the hit and object classes */
class object;

class hit {
    private:
        ray* generator;
        rt::vector point;
        rt::vector normal;
        const object* hit_object;
        bool inward;

    public:
        /* Main constructor */
        hit(ray* generator, const rt::vector& point, const rt::vector& normal, const object*& hit_object);

        hit(ray* generator, const rt::vector& point, const rt::vector& normal, const object*& hit_object, const bool inward);

        hit();

        /* Accessors */
        inline const rt::vector& get_point() const {
            return point;
        }
        
        inline const rt::vector& get_normal() const {
            return normal;
        }

        inline const object* get_object() const {
            return hit_object;
        }

        inline bool is_inward() const {
            return inward;
        }

        inline ray* get_generator_ray() const {
            return generator;
        }
};