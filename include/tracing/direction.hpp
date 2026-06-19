#pragma once

#include "light/hit.hpp"
#include "auxiliary/randomgen.hpp"
#include "auxiliary/utils.hpp"

class direction {
    public:

        /* Returns the reflected ray at the point of contact */
        // static ray get_reflected_ray();

        template<orientation_type ray_orientation>
        /* Returns the interpolated direction between the normal and the reflected direction */
        /* inward = ((direction | normal) <= 0) */
        static rt::vector central_reflected(const hit& h, const rt::vector& normal, const real smoothness) {
            constexpr real correcting_factor = (ray_orientation == orientation_type::Inward) ? 1.0_r : -1.0_r;
            constexpr real two_corr_f = -2.0_r * correcting_factor;

            const rt::vector& u = h.get_generator_ray()->direction;
            const real two_cos = two_corr_f * (u | normal);
            //return (smoothness * (2.0_r * cos - 1.0_r) + 1.0_r) * right_normal + smoothness * u;
            return fma(u, smoothness, ((smoothness * (two_cos - 1.0_r) + 1.0_r) * correcting_factor) * normal);
        }

        /* Returns the interpolated direction between the normal and the reflected direction */
        /* inward = ((direction | normal) <= 0) */
        static rt::vector central_reflected(const hit& h, const rt::vector& normal, real reflectivity, orientation_type ray_orientation);

        enum class angle {
            Pi, Pi_over_2
        };

        /* Returns a random unit direction in the cone of center central_dir, within solid angle theta_max */
        template <angle theta_max>
        static rt::vector random(const randomgen& rg, const rt::vector& central_dir = rt::vector(0,0,0)) {

            using enum angle;
            if constexpr (theta_max == Pi || theta_max == Pi_over_2) {
                const real cos_theta = 2.0_r * rg.random_ratio() - 1.0_r;
                const real sin_theta = sqrt(1.0_r - cos_theta * cos_theta);
                const real phi = rg.random_angle();
                
                const rt::vector v(
                    cos(phi) * sin_theta,
                    sin(phi) * sin_theta,
                    cos_theta
                );

                if constexpr (theta_max == Pi) {
                    return v;
                }
                else {
                    return std::signbit(v | central_dir) ? (-1.0_r) * v : v;
                }
            }
        }

        // Run-time
        static rt::vector random(const randomgen& rg, const rt::vector& central_dir, real theta_max);

        /* Refraction */

        struct sin_refracted_output {
            rt::vector vx;
            real sin_theta_2_sq;
        };

        /* Returns sin(theta_2) squared, where theta_2 is the refracted angle
            Is precomputed to determine whether the ray is refracted or internally reflected */
        static inline sin_refracted_output get_sin_refracted(const hit& h, const rt::vector& normal,
            const real current_refr_i, const real surface_refr_i) {

            sin_refracted_output out;

            /* See get_refracted_direction below */
            const rt::vector& dir = h.get_generator_ray()->direction;
            /* It should be (current_refr_i / surface_refr_i) * ((((-1)*(dir | right_normal)) * right_normal) + dir)
            where right_normal = inward ? normal : (-1) * normal,
            but the next line is equivalent */
            //const rt::vector vx = (current_refr_i / surface_refr_i) * ((((-1.0_r) * (dir | normal)) * normal) + dir);
            out.vx = (current_refr_i / surface_refr_i) * fma(normal, (-1.0_r) * (dir | normal), dir);
            out.sin_theta_2_sq = out.vx.normsq();
            return out;
        }

        /* Returns the refracted direction */
        static inline rt::vector refracted(const rt::vector& normal, const sin_refracted_output& sin_refr,
            const orientation_type ray_orientation) {
            
            /* Factor to apply to normal to obtain the normal outward the surface of contact,
            so that (dir | a * normal) <= 0 */
            // const real a = inward ? 1 : (-1);

            /* Snell-Descartes law */
            /* If the angle between (-dir) and a*normal is theta_1,
            the angle between the refracted direction v and (-a*normal) is theta_2,
            then current_refr_i * sin(theta_1) = surface_refr_i * sin(theta_2)

            sin(theta_1) = |(-dir | (a * normal)) * (a*normal) - (-dir)|
            So sin(theta_2) = (current_refr_i / surface_refr_i) * |(dir | (a * normal)) * (a*normal) - dir|

            The refracted direction v can be decomposed into v = vx + vy,
            where vx is coplanar to dir and normal, orthogonal to normal:
            vx = (current_refr_i / surface_refr_i) * ((dir | a*normal) * (a*normal) - dir)
            and vy = sqrt(1 - vx.normsq()) * (-a)*normal,
            so that v is a unit vector
            */

            //return vx + sqrt(1.0_r - sin_theta_2_sq) * (inward ? (-1.0_r) * normal : normal);
            using enum orientation_type;
            const real scale = (ray_orientation == Inward ? -1.0_r : 1.0_r) * sqrt(1.0_r - sin_refr.sin_theta_2_sq);
            return fma(normal, scale, sin_refr.vx);
        }

        /* Returns a random unit direction in the cone whose center is the refracted direction, within solid angle refraction_scattering * pi */
        static inline rt::vector random_refracted(const randomgen& rg, const real refraction_scattering,
            const rt::vector& normal,
            const sin_refracted_output& sin_refr,
            const orientation_type ray_orientation) {

            const rt::vector refr_dir = refracted(normal, sin_refr, ray_orientation);
            return random(rg, refr_dir, refraction_scattering * (PI / 2.0));
        }

        /* Computes the Fresnel coefficient Kr */
        static real get_fresnel(const hit& h, const rt::vector& normal, real sin_theta_2_sq, real refr_1, real refr_2);

        /* Compute Schlick's approximation of Fresnel coefficient Kr */
        static real get_schlick(const hit& h, const rt::vector& normal, real refr_1, real refr_2);
};