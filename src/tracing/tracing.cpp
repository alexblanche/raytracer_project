#include "tracing/tracing.hpp"
#include "tracing/directions.hpp"
#include "auxiliary/custom_stack.hpp"
#include "auxiliary/utils.hpp"

#include <cmath>

/* ******************************************************************** */
/* *************************** Path tracing *************************** */

enum class update_option {
    UpdateColorMaterials, DoNotUpdateColorMaterials
};
using enum update_option;

struct accumulators {
    rt::color color_materials;
    rt::color emitted_colors;

    accumulators() :
        color_materials(rt::WHITE),
        emitted_colors(rt::BLACK) {}


    [[nodiscard]] inline rt::color combine(const rt::color& color) const {
        return fma(
            color_materials,
            color,
            emitted_colors);
    }

    inline void update(const material& m, const rt::color& local_color,
        const update_option update_option) {

        emitted_colors =
            m.is_emissive() ?
                  combine(m.get_color() * m.get_emission_intensity())
                : emitted_colors;
        color_materials =
            (update_option == UpdateColorMaterials) ?
                  color_materials * local_color
                : color_materials;
    }

    inline void update_emitted_col(const material& m) {
        emitted_colors = combine(m.get_color() * m.get_emission_intensity());
    }
    inline void update_color_mat(const rt::color& local_color) {
        color_materials *= local_color;
    }
};

/** Auxiliary functions **/

constexpr real BIAS_NORM = 1.0E-3_r;

// Direct bias when the orientations of the ray and the bias are opposite (bias along the normal)
// Inverted otherwise
enum class bias_type {
    Direct, Inverted
};

/* Auxiliary function that applies a bias of 1.0E-3 times the normal to the ray position,
   outward the surface contact point if outward_bias is true (so in the direction of the normal),
   inward otherwise (in the opposite direction to the normal) */
[[nodiscard]] inline rt::vector get_bias(const rt::vector& hit_point, const rt::vector& normal,
    const orientation_type ray_orientation, const orientation_type bias_orientation) {

    return
        fma(
            normal,
            (ray_orientation != bias_orientation) ? BIAS_NORM : (-BIAS_NORM),
            hit_point
        );
}

template <orientation_type ray_orientation, orientation_type bias_orientation>
[[nodiscard]] inline rt::vector get_bias(const rt::vector& hit_point, const rt::vector& normal) {

    constexpr real right_bias = (ray_orientation != bias_orientation ? 1.0_r : -1.0_r) * BIAS_NORM;
    return fma(normal, right_bias, hit_point);
}


/* Auxiliary function that handles the specular reflective case */
// Run-time
[[nodiscard]] static ray specular_reflective_case(const hit& h, const randomgen& rg, const real smoothness,
    const rt::vector& local_normal, const orientation_type ray_orientation) {

    const rt::vector central_dir = get_central_reflected_direction(h, local_normal, smoothness, ray_orientation);
                    
    /* Direction according to Lambert's cosine law */
    const rt::vector dir = (smoothness >= 1.0_r) ?
          central_dir
        : (fma(random_direction<angle::Pi>(rg, central_dir), 1.0_r - smoothness, central_dir)).unit();

    // Here: be careful not to go below the surface, when its local normal is almost parallel to the surface (cap the max angle to the local_normal)

    /* Apply the bias outward the surface */
    const rt::vector origin = get_bias(h.get_point(), h.get_normal(), ray_orientation, orientation_type::Outward);

    return ray(origin, dir);
}

// Compile-time
template<orientation_type ray_orientation>
[[nodiscard]] static ray specular_reflective_case(const hit& h, const randomgen& rg, const real smoothness,
    const rt::vector& local_normal) {

    const rt::vector central_dir = get_central_reflected_direction<ray_orientation>(h, local_normal, smoothness);

    /* Direction according to Lambert's cosine law */
    const rt::vector dir = (smoothness >= 1.0_r) ?
        central_dir
        :
        (fma(random_direction<angle::Pi>(rg, central_dir), 1.0_r - smoothness, central_dir)).unit();
        
    // Here: be careful not to go below the surface, when its local normal is almost parallel to the surface (cap the max angle to the local_normal)

    /* Apply the bias outward the surface */
    const rt::vector origin = get_bias<ray_orientation, orientation_type::Outward>(h.get_point(), h.get_normal());
    return ray(origin, dir);
}


/* Auxiliary function that handles the diffuse reflective case */
// Run-time
[[nodiscard]] ray diffuse_case(const hit& h, const rt::vector& local_normal, const randomgen& rg, const orientation_type ray_orientation) {

    using enum orientation_type;
    const rt::vector dir(
        ((ray_orientation == Inward ?
            local_normal : (-1.0_r) * local_normal)
            + random_direction<angle::Pi>(rg, local_normal)
        ).unit()
    );
    // Here: be careful not to go below the surface, when its local normal is almost parallel to the surface (cap the max angle to the local_normal)

    /* Apply the bias outward the surface */
    const rt::vector origin = get_bias(h.get_point(), h.get_normal(), ray_orientation, Outward);
    return ray(origin, dir);
}


/* Auxiliary function that handles the refractive case */
[[nodiscard]] ray refractive_case(const hit& h, const randomgen& rg, const real scattering,
    const rt::vector& local_normal,
    const rt::vector& vx, const real sin_theta_2_sq, const orientation_type ray_orientation,
    real& refr_index, const real next_refr_i) {

    /* Setting the refracted direction */
    const rt::vector dir(
        is_not_zero(scattering) ?
            get_random_refracted_direction(
                rg,
                scattering,
                local_normal,
                vx, sin_theta_2_sq, ray_orientation
            )
            :
            get_refracted_direction(local_normal, vx, sin_theta_2_sq, ray_orientation)
    );

    /* Updating the refraction index */
    refr_index = next_refr_i;

    /* Apply the bias inward the surface */
    const rt::vector origin = get_bias(h.get_point(), h.get_normal(), ray_orientation, orientation_type::Inward);
    return ray(origin, dir);
}

[[nodiscard]] inline rt::color background_case(const scene& scene, const ray& r,
    const accumulators& acc) {

    /* Determining the pixel of the background texture to display */
    /* Determining the spherical coordinates of the direction,
        then the UV-coordinates in the 360 image */

    return acc.combine(
        scene.background.has_texture() ?
              scene.background.get_color(r.direction)
            : scene.background.get_color()
    );
}


/* Path tracing function */

/*  Computes the hit of the given ray on the closest object,
    then launches one ray, in a direction and with a color depending on the surface material,
    until it is too dim, or a light-emitting object is hit, or the maximum number of bounces is reached. */

/* In recursive form, the light equation is of the form u(n) = a(n) * u(n-1) + b(n),
   in iterative form, we have an accumulator color_materials of the product of the a(k), k=n..,
   and an accumulator (emitted_colors) of the (product of a(j), j=n..k) * b(k). */

rt::color pathtrace(const ray& init_ray, const scene& scene, const randomgen& rg, const unsigned int bounce,
    const russian_roulette_mode russian_roulette_mode,
    const real init_refr_index) {

    accumulators acc;
        
    real refr_index = init_refr_index;
    static thread_local custom_stack<real> refr_stack(20);
    refr_stack.set_empty();

    const bvh_option bvh = (scene.polygons_per_bounding != 0) ? bvh_option::Enabled : bvh_option::Disabled;

    const bool russian_roulette = russian_roulette_mode == russian_roulette_mode::Enabled;

    ray r(init_ray);

    for (unsigned int i = 0; i < bounce; i++) {

        if (russian_roulette) {
            const real avg = acc.color_materials.get_average_ratio();
            if (avg < 1.0_r) {
                if (rg.random_ratio() <= 1.0_r - avg)
                    return acc.emitted_colors;
                
                acc.color_materials /= avg;
            }
        }

        const std::optional<hit> opt_h = scene.find_closest(r, bvh);

        if (not opt_h.has_value()) /* No object hit: background color or background texture */
            return background_case(scene, r, acc);
            
        const hit& h = opt_h.value();
        const object* obj = h.get_object();
        const material& m = scene.material_set[obj->get_material_index()];

        /* Full-intensity light source reached */
        if (m.is_emissive() && m.get_emission_intensity() >= 1.0_r) {

            const rt::color& color = obj->is_textured() ?
                scene.sample_texture(obj->get_texture_info_index(), obj->get_barycentric(h.get_point()))
                :
                m.get_color();

            return acc.combine(color * m.get_emission_intensity());
        }

        /* The ray can either be transmitted (and refracted) through the surface,
            or reflected in three ways: specularly, diffusely, or in the case of total internal reflection,
            when the ray hits a surface of lower refraction index at an angle greater than the critical angle.
        */
        const orientation_type ray_orientation = h.is_inward();

        // Contains the material local color, the local normal and (soon) the reflectivity and displacement
        const map_sample ms = (obj->is_textured()) ?
            scene.sample_maps(obj->get_texture_info_index(), obj->get_barycentric(h.get_point()),
                m.get_color(), h.get_normal(), m.get_smoothness())
            :
            map_sample(m.get_color(), h.get_normal()); // reflectivity, displacement);

        const rt::vector normal = (obj->is_textured() && scene.get_texture_info(obj).has_normal_information()) ?
            obj->compute_normal_from_map(
                ms.normal_map_vector,
                h.get_normal(),
                scene.get_texture_info(obj)
            )
            :
            h.get_normal();

        const rt::color& color = ms.texture_color;
        const real smoothness = m.get_smoothness(); // ms.smoothness;
        //

        if (m.is_opaque()) {
            /* Diffuse or specular reflection */

            /* Testing whether the ray is reflected specularly or diffusely */
            if (m.is_specular()) {
                
                /* Specular bounce */

                const bool is_specular_bounce = rg.random_ratio() <= m.get_reflectivity();
                const real specular_smoothness = is_specular_bounce ? smoothness : 0.0_r;
                
                r = specular_reflective_case(h, rg, specular_smoothness, normal, ray_orientation);

                /* We update color_materials only if the material reflects colors (like a christmas tree ball),
                otherwise the reflection has the original color (like a tomato) */
                const update_option update_option =
                    (!is_specular_bounce || m.does_reflect_color()) ?
                      UpdateColorMaterials
                    : DoNotUpdateColorMaterials;
                acc.update(m, color, update_option);
            }
            else {

                /* Diffuse bounce */

                r = diffuse_case(h, normal, rg, ray_orientation);
                
                //acc.update(m, color, true);
                if (m.is_emissive()) acc.update_emitted_col(m);
                acc.update_color_mat(color);
            }
        }
        else {
            /* Transmission or reflection, depending on the Fresnel coefficients Kr, Kt
                Kr is the probability that the ray is reflected, Kt the probability that the ray is transmitted */

            /* Computation of the new refraction index */
            // const bool inward = ray_orientation == orientation_type::Inward;
            real next_refr_i;
            using enum orientation_type;
            switch (ray_orientation) {
                case Inward:
                    next_refr_i = m.get_refraction_index();
                    if (refr_index != 1.0_r) {
                        refr_stack.push(refr_index);
                    }
                    break;
                
                case Outward:
                    next_refr_i = (not refr_stack.empty()) ? refr_stack.pop() : 1.0_r;
                    break;
            }

            /* Computation of the Fresnel coefficient */

            if ((ray_orientation == Inward)
                &&
                rg.random_ratio() * m.get_transparency() <= get_schlick(h, normal, refr_index, next_refr_i)) {
            
                /* The ray is reflected */
                
                /* Is it a pure specular or a mix of specular and diffuse just like in the previous case? */
                r = specular_reflective_case<Inward>(h, rg, smoothness, normal);

                //acc.update(m, color, false);
                if (m.is_emissive()) acc.update_emitted_col(m);
            }
            else {

                /* Pre-computation of the refracted direction */
                real sin_theta_2_sq;
                const rt::vector vx = get_sin_refracted(h, normal, refr_index, next_refr_i, sin_theta_2_sq);
                
                /* The ray is transmitted */

                /* Determination of whether the ray is transmitted (refracted) or in total interal reflection */
                if (sin_theta_2_sq >= 1.0_r) {
                    /* Total internal reflection */

                    r = specular_reflective_case(h, rg, smoothness, normal, ray_orientation);
                    //acc.update(m, color, false);
                    if (m.is_emissive()) acc.update_emitted_col(m);
                }
                else {
                    /* Transmission */

                    r = refractive_case(h, rg, m.get_refraction_scattering(), normal,
                        vx, sin_theta_2_sq, ray_orientation, refr_index, next_refr_i);
                    // acc.update(m, color, true);
                    if (m.is_emissive()) acc.update_emitted_col(m);
                    acc.update_color_mat(color);
                }
            }
        }
        
    }

    /* Maximum number of bounces reached: the final color is black */
    return acc.emitted_colors;
}






