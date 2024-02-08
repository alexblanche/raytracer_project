#pragma once


#include "../../screen/headers/color.hpp"
#include "vector.hpp"
#include "ray.hpp"
#include <vector>
#include "../../auxiliary/headers/randomgen.hpp"

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
        bool is_hit_bool;

    public:
        /* Main constructor */
        hit(const ray& g, const rt::vector& p, const rt::vector& n, const unsigned int i);

        hit();

        /* Accessors */
        ray get_ray() const;

        rt::vector get_point() const;
        
        rt::vector get_normal() const;

        unsigned int get_obj_index() const;

        bool is_hit() const;

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