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
        rt::vector flat_normal;
        const object* hit_object;

    public:
        /* Main constructor */
        hit(ray*& generator, const rt::vector& point, const rt::vector& normal, const object*& hit_object);

        hit(ray*& generator, const rt::vector& point, const rt::vector& normal, const rt::vector& flat_normal, const object*& hit_object);

        hit();

        /* Accessors */
        inline const rt::vector& get_point() const {
            return point;
        }
        
        inline const rt::vector& get_normal() const {
            return normal;
        }

        inline const rt::vector& get_flat_normal() const {
            return flat_normal;
        }

        inline const object* get_object() const {
            return hit_object;
        }

        /* Reflection */

        /* Returns the reflected ray at the point of contact */
        // ray get_reflected_ray() const;

        /* Returns the interpolated direction between the normal and the reflected direction */
        /* inward = ((direction | normal) <= 0) */
        rt::vector get_central_reflected_direction(const real reflectivity, const bool inward) const;

        /* Returns a vector of n random reflected ray in the cone of center hit::reflect_ray(),
           within solid angle theta_max */
        // std::vector<ray> random_reflect(const size_t n, randomgen& rg,
        //     const rt::vector& central_dir, const real theta_max) const;

        /* Returns a random unit direction in the cone of center central_dir, within solid angle theta_max */
        rt::vector random_direction(randomgen& rg, const rt::vector& central_dir,
            const real theta_max) const;

        /* Refraction */

        /* Returns sin(theta_2), where theta_2 is the refracted angle
           Is precomputed to determine whether the ray is refracted or internally reflected */
        rt::vector get_sin_refracted(const real current_refr_i, const real surface_refr_i,
            real& sin_theta_2) const;

        /* Returns the refracted direction */
        rt::vector get_refracted_direction(const rt::vector& vx, const real sin_theta_2, const bool inward) const;

        /* Returns a random unit direction in the cone whose center is the refracted direction, within solid angle refraction_scattering * pi */
        rt::vector get_random_refracted_direction(randomgen& rg, const real refraction_scattering,
            const rt::vector& vx, const real sin_theta_2_sq, const bool inward) const;

        /* Computes the Fresnel coefficient Kr */
        real get_fresnel(const real sin_theta_2_sq, const real refr_1, const real refr_2) const;

        /* Compute Schlick's approximation of Fresnel coefficient Kr */
        real get_schlick(const real refr_1, const real refr_2) const;
};