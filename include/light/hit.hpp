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
        ray generator;
        rt::vector point;
        rt::vector normal;
        const object* hit_object;
        bool is_hit_bool;

    public:
        /* Main constructor */
        hit(const ray& generator, const rt::vector& point, const rt::vector& normal, const object*& hit_object);

        hit();

        /* Accessors */
        inline ray get_ray() const {
            return generator;
        }

        inline rt::vector get_point() const {
            return point;
        }
        
        inline rt::vector get_normal() const {
            return normal;
        }

        inline const object* get_object() const {
            return hit_object;
        }

        inline bool object_hit() const {
            return is_hit_bool;
        }

        /* Reflection */

        /* Returns the reflected ray at the point of contact */
        ray get_reflected_ray() const;

        /* Returns the interpolated direction between the normal and the reflected direction */
        rt::vector get_central_direction(const double reflectivity) const;

        /* Returns a vector of n random reflected ray in the cone of center hit::reflect_ray(),
           within solid angle theta_max */
        std::vector<ray> random_reflect(const unsigned int n, randomgen& rg,
            const rt::vector& central_dir, const double theta_max) const;

        /* Returns the direction of a random reflected ray in the cone of center hit::reflect_ray(),
           within solid angle theta_max */
        rt::vector random_reflect_single(randomgen& rg, const rt::vector& central_dir,
            const double theta_max) const;
};