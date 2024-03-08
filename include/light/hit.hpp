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
        bool is_hit_bool;

    public:
        /* Main constructor */
        hit(ray*& generator, const rt::vector& point, const rt::vector& normal, const object*& hit_object);

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

        inline bool object_hit() const {
            return is_hit_bool;
        }

        /* Reflection */

        /* Returns the reflected ray at the point of contact */
        // ray get_reflected_ray() const;

        /* Returns the interpolated direction between the normal and the reflected direction */
        /* inward = ((direction | normal) <= 0) */
        rt::vector get_central_reflected_direction(const double reflectivity, const bool inward) const;

        /* Returns a vector of n random reflected ray in the cone of center hit::reflect_ray(),
           within solid angle theta_max */
        std::vector<ray> random_reflect(const unsigned int n, randomgen& rg,
            const rt::vector& central_dir, const double theta_max) const;

        /* Returns a random direction in the cone of center central_dir, within solid angle theta_max */
        rt::vector random_direction(randomgen& rg, const rt::vector& central_dir,
            const double theta_max) const;

        /* Refraction */

        /* Returns the refracted direction */
        rt::vector get_refracted_direction(const double current_refr_i, const double surface_refr_i) const;

        /* Returns a random direction in the cone whose center is the refracted direction, within solid angle refraction_scattering * pi */
        rt::vector get_random_refracted_direction(randomgen& rg,
            const double current_refr_i, const double surface_refr_i, const double refraction_scattering) const;
};