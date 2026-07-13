#include "tracing/tracing.hpp"
#include "tracing/direction.hpp"
#include "auxiliary/utils.hpp"
#include "auxiliary/stack_based_custom_stack.hpp"

#include <cmath>

/* ******************************************************************** */
/* *************************** Path tracing *************************** */

using enum orientation_type;
using enum direction::angle;

/** Auxiliary functions **/

/* Auxiliary function that handles the diffuse reflective case */
[[nodiscard]] inline ray worker::diffuse_case(const hit& h, const rt::vector& local_normal) const {

    const rt::vector dir(
        ((h.get_orientation() == Inward ? local_normal : (-1.0_r) * local_normal)
          + direction::random<Pi>(rg)
        ).unit()
    );
    // Here: be careful not to go below the surface, when its local normal is almost parallel to the surface (cap the max angle to the local_normal)

    /* Apply the bias outward the surface */
    const rt::vector origin = h.biased_point(Outward);
    return ray(origin, dir);
}


/* Auxiliary function that handles the specular reflective case */
// Run-time
[[nodiscard]] inline ray worker::specular_reflective_case(const hit& h, const rt::vector& direction,
    const real smoothness, const rt::vector& local_normal) const {

    /* Direction according to Lambert's cosine law */

    const rt::vector central_dir = direction::central_reflected(direction, local_normal, smoothness, h.get_orientation());
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
              direction::random_refracted(rg, scattering, local_normal, sin_refr, h.get_orientation())
            : direction::refracted(local_normal, sin_refr, h.get_orientation())
    );
}

/* Determining the pixel of the background texture to display */
[[nodiscard]] inline rt::color worker::background_case(const rt::vector& direction, const accumulators& acc) const {

    const rt::color& color = scene_.background.has_texture() ?
          scene_.background.get_color(direction)
        : scene_.background.get_color();

    return acc.combine(color);
}



struct bounce_vectors {
    const rt::vector& direction;
    const rt::vector& normal;
};


void worker::process_bounce(const bounce_parameters& param, path_parameters& out) const {
    
    const auto& [ h, m, normal, color, smoothness ] = param;
    auto& [ r, acc, refr_index ] = out;

    if (m.is_opaque()) {
        /* Diffuse or specular reflection */

        /* Testing whether the ray is reflected specularly or diffusely */
        if (m.is_specular()) {
            
            /* Specular bounce */

            const bool is_specular_bounce = rg.random_ratio() <= m.get_reflectivity();
            const real specular_smoothness = is_specular_bounce ? smoothness : 0.0_r;
            
            r = specular_reflective_case(h, r.direction, specular_smoothness, normal);

            /* We update color_materials only if the material reflects colors (like a christmas tree ball),
            otherwise the reflection has the original color (like a tomato) */
            if (!is_specular_bounce || m.does_reflect_color())
                acc.update_color_mat(color);
        }
        else {

            /* Diffuse bounce */

            r = diffuse_case(h, normal);
            acc.update_color_mat(color);
        }
    }
    else {
        /* Transmission or reflection, depending on the Fresnel coefficients Kr, Kt
            Kr is the probability that the ray is reflected, Kt the probability that the ray is transmitted */

        const auto compute_next_refraction_index = [&] {

            switch (h.get_orientation()) {
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

            const real fresnel = direction::get_schlick(r.direction, normal, refr_index, next_refr_i);
            return rg.random_ratio() * m.get_transparency() <= fresnel;
        };

        if ((h.get_orientation() == Inward) && is_fresnel_reflection()) {
        
            /* The ray is reflected */
            
            /* Is it a pure specular or a mix of specular and diffuse just like in the previous case? */
            r = specular_reflective_case<Inward>(h, r.direction, smoothness, normal);
        }
        else {

            /* Pre-computation of the refracted direction */
            const auto sin_refr = direction::get_sin_refracted(r.direction, normal, refr_index, next_refr_i);
            const auto& [ _, sin_theta_2_sq ] = sin_refr;

            /* Determination of whether the ray is transmitted (refracted) or in total interal reflection */
            if (sin_theta_2_sq >= 1.0_r) {
                /* Total internal reflection */

                r = specular_reflective_case(h, r.direction, smoothness, normal);
            }
            else {
                /* Transmission */

                r = refractive_case(h, m.get_refraction_scattering(), normal, sin_refr, refr_index, next_refr_i);
                acc.update_color_mat(color);
            }
        }
    }

    if (m.is_emissive())
        acc.update_emitted_col(m);
}

[[nodiscard]] inline rt::color worker::full_intensity_case(accumulators& acc,
    const object* obj, const rt::vector& hit_point, const material& m) const {
    
    const rt::color& color = obj->is_textured() ?
          scene_.sample_texture(obj->get_texture_info_index(), obj->get_barycentric(hit_point))
        : m.get_color();

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

        const std::optional<hit> opt_h = scene_.find_closest(r, bvh);

        /* No object hit: background color or background texture */
        if (not opt_h.has_value()) 
            return background_case(r.direction, acc);
        
        
        /* Object hit */

        const hit&          h   = opt_h.value();
        const object* const obj = h.get_object();
        const material&     m   = scene_.material_set[obj->get_material_index()];

        /* Full-intensity light source reached */
        if (m.is_emissive() && m.get_emission_intensity() >= 1.0_r)
            return full_intensity_case(acc, obj, h.get_point(), m);

        
        /* The ray can either be transmitted (and refracted) through the surface,
            or reflected in three ways: specularly, diffusely, or in the case of total internal reflection,
            when the ray hits a surface of lower refraction index at an angle greater than the critical angle.
        */

        // map_sample contains the material local color, the local normal and (soon) the reflectivity and displacement
        const map_sample ms = (obj->is_textured()) ?
              scene_.sample_maps(
                obj->get_texture_info_index(), obj->get_barycentric(h.get_point()),
                m.get_color(), h.get_normal(), m.get_smoothness())
            : map_sample(m.get_color(), h.get_normal()); // reflectivity, displacement);

        const bounce_parameters param = {
            .h = h,
            .m = m,
            .normal = (obj->is_textured() && scene_.get_texture_info(obj).has_normal_information()) ?
                  obj->compute_normal_from_map(ms.normal_map_vector, h.get_normal(), scene_.get_texture_info(obj))
                : h.get_normal(),
            .color = ms.texture_color,
            .smoothness = m.get_smoothness() // ms.smoothness;
        };
        
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