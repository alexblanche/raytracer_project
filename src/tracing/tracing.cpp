#include "tracing/tracing.hpp"

#include "tracing/direction.hpp"
#include "auxiliary/utils.hpp"
#include "auxiliary/stack_based_custom_stack.hpp"

/* ******************************************************************** */
/* *************************** Path tracing *************************** */

using enum ray_orientation_type;
using enum direction::angle;

/** Auxiliary functions **/

/* Auxiliary function that handles the diffuse reflective case */
[[nodiscard]] inline ray worker::diffuse_case(const hit& h, const rt::vector& local_normal) const {

    const rt::vector dir(
        ((h.get_ray_orientation() == Inward ? local_normal : (-1.0_r) * local_normal)
          + direction::random<Pi>(rg)
        ).unit()
    );

    /* Apply the bias outward the surface */
    const rt::vector origin = h.biased_point(Outward);
    return ray(origin, dir);
}


/* Auxiliary function that handles the specular reflective case */
// Run-time
[[nodiscard]] inline ray worker::specular_reflective_case(const hit& h, const direction::bounce_vectors& bounce_v,
    const real smoothness) const {

    const rt::vector central_dir = direction::central_reflected(bounce_v, smoothness, h.get_ray_orientation());
    return ray( 
        /* origin */
        h.biased_point(Outward),

        /* direction */
        (smoothness >= 1.0_r) ?
              central_dir
            : (fma(direction::random<Pi>(rg), 1.0_r - smoothness, central_dir)).unit()
    );
}

/* Auxiliary function that handles the refractive case */
[[nodiscard]] inline ray worker::refractive_case(const hit& h, const real scattering,
    const rt::vector& local_normal, const direction::sin_refracted_output& sin_refr,
    real& refr_index, const real next_refr_i) const {
        
    /* Updating the refraction index */
    refr_index = next_refr_i;

    return ray(
        /* origin */
        h.biased_point(Inward),

        /* direction */
        is_not_zero(scattering) ?
              direction::random_refracted(rg, scattering, local_normal, sin_refr, h.get_ray_orientation())
            : direction::refracted(local_normal, sin_refr, h.get_ray_orientation())
    );
}

/* Determining the pixel of the background texture to display */
[[nodiscard]] inline rt::color worker::background_case(const rt::vector& direction, const accumulators& acc) const {

    const rt::color& color = scene_.mapping_containers.background.get_color(direction);
    return acc.combine(color);
}

[[nodiscard]] inline bool worker::specular_bounce(const bounce_parameters& param, const direction::bounce_vectors& bounce_v) const {
    /* Testing whether the ray is reflected specularly or diffusely */

    const auto& [ _, m, _, _, _, double_bounce ] = param;

    real bounce_probability = m.has_fresnel() ?
          direction::get_fresnel(bounce_v, 1.0_r, m.get_refraction_index())
        : m.get_reflectivity();

    // Double glazing: bouncing back and forth between the two panes
    if (double_bounce)
        bounce_probability = 2 * bounce_probability / (1 + bounce_probability);

    return rg.random_ratio() <= bounce_probability;
}

void worker::process_bounce(const bounce_parameters& param, path_parameters& out) const {
    
    const auto& [ h, m, normal, color, smoothness, double_bounce ] = param;
    auto& [ r, acc, refr_index ] = out;

    const direction::bounce_vectors bounce_v(r.direction, normal);
    bool update_color = false;

    if (m.is_opaque()) {
        /* Diffuse or specular reflection */

        if (m.is_specular() && specular_bounce(param, bounce_v)) {
            
            /* Specular bounce */
            r = specular_reflective_case(h, bounce_v, smoothness);

            /* We update color_materials only for metals,
               otherwise the reflection has the original color (dielectric) */
            update_color = m.does_reflect_color();
        }
        else {

            /* Diffuse bounce */
            r = diffuse_case(h, normal);
            update_color = true;
        }
    }
    else {
        /* Transmission or reflection, depending on the Fresnel coefficients Kr, Kt
            Kr is the probability that the ray is reflected, Kt the probability that the ray is transmitted */

        const auto compute_next_refraction_index = [&] {

            switch (h.get_ray_orientation()) {
                case Inward:
                    if (refr_index != 1.0_r)
                        refr_stack.push(refr_index);
                    return m.get_refraction_index();
                
                case Outward:
                    return (not refr_stack.empty()) ? refr_stack.pop() : 1.0_r;

                default: throw;
            }
        };

        const real next_refr_i = compute_next_refraction_index();

        const auto is_fresnel_reflection = [&] {

            const real fresnel = direction::get_fresnel(bounce_v, refr_index, next_refr_i);
            return rg.random_ratio() * m.get_transparency() <= fresnel;
        };

        if ((h.get_ray_orientation() == Inward) && is_fresnel_reflection()) {
        
            /* Fresnel reflection */
            r = specular_reflective_case<Inward>(h, bounce_v, smoothness);
        }
        else {

            /* Pre-computation of the refracted direction */
            const auto refr_info = direction::get_sin_refracted(bounce_v, refr_index, next_refr_i);
            const auto& [ _, sin_theta_2_sq ] = refr_info;

            /* Test for total interal reflection */
            if (sin_theta_2_sq >= 1.0_r) {

                /* Total internal reflection */
                r = specular_reflective_case(h, bounce_v, smoothness);
            }
            else {

                /* Transmission */
                r = refractive_case(h, m.get_refraction_scattering(), normal, refr_info, refr_index, next_refr_i);
                update_color = true;
            }
        }
    }

    if (update_color)
        acc.update_color_mat(color);

    if (m.is_emissive())
        acc.update_emitted_col(m);
}

[[nodiscard]] inline rt::color worker::full_intensity_case(const accumulators& acc,
    const hit& h, const material& m) const {
    
    const rt::color& color = scene_.sample_color(h, m);
    return acc.combine(color * m.get_emission_intensity());
}

/* Path tracing function */

/*  Computes the hit of the given ray on the closest object,
    then launches one ray, in a direction and with a color depending on the surface material,
    until it is too dim, or a light-emitting object is hit, or the maximum number of bounces is reached. */

/* In recursive form, the light equation is of the form u(n) = a(n) * u(n-1) + b(n),
   in iterative form, we have an accumulator color_materials of the product of the a(k), k = n...,
   and an accumulator (emitted_colors) of the (product of a(j), j = n..k) * b(k). */

rt::color worker::pathtrace(const ray& init_ray) const {
    
    refr_stack.set_empty();

    path_parameters path_param = {
        .r = init_ray,
        .acc = {},
        .refr_index = init_refr_index
    };
    auto& [ r, acc, refr_index ] = path_param;

    for (unsigned int i = 0; i < bounce; i++) {

        const std::optional<hit> opt_h = scene_.find_intersection(r);

        /* No object hit: background color or background texture */
        if (not opt_h.has_value()) 
            return background_case(r.direction, acc);
        
        
        /* Object hit */

        const hit&          h   = opt_h.value();
        const object* const obj = h.get_object();
        const material&     m   = scene_.mapping_containers.material_set[obj->get_material_index()];

        /* Full-intensity light source reached */
        if (m.is_emissive() && m.get_emission_intensity() >= 1.0_r)
            return full_intensity_case(acc, h, m);

        
        /* The ray can either be transmitted (and refracted) through the surface,
            or reflected in three ways: specularly, diffusely, or in the case of total internal reflection,
            when the ray hits a surface of lower refraction index at an angle greater than the critical angle.
        */

        // map_sample contains the local information: texture color and normal (and soon: smoothness and displacement)
        const auto& [ color, normal ] = scene_.sample_maps(h, m);
        
        ////
        const bool double_bounce = false; // (obj->get_material_index() == 11); // Windshield of Porsche 2016
        ////
        const bounce_parameters param = { h, m, normal, color, m.get_smoothness(), double_bounce }; // or ms.smoothness

        process_bounce(param, path_param);

        if (russian_roulette == russian_roulette_mode::Enabled) {
            const real avg = acc.color_materials.get_average_ratio();
            if (avg < 1.0_r) {
                if (rg.random_ratio() <= 1.0_r - avg)
                    return acc.emitted_colors;
                acc.color_materials /= avg;
            }
        }
    }

    /* Maximum number of bounces reached: the final color is black */
    return acc.emitted_colors;
}