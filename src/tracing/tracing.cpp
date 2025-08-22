#include <cmath>
#include "light/ray.hpp"
#include "screen/color.hpp"
#include "scene/objects/object.hpp"
// #include "scene/objects/polygon.hpp"
#include "scene/scene.hpp"
#include "scene/objects/bounding.hpp"
#include "scene/material/texture.hpp"

#include "tracing/directions.hpp" 

//#include <stack>
#include "auxiliary/custom_stack.hpp"

#define PI 3.14159265358979323846f


/* ******************************************************************** */
/* *************************** Path tracing *************************** */

struct accumulators {
    rt::color color_materials;
    rt::color emitted_colors;

    accumulators() :
        color_materials(rt::color::WHITE),
        emitted_colors(rt::color::BLACK) {}


    inline rt::color combine(const rt::color& color) const {
        return fma(
            color_materials,
            color,
            emitted_colors);
    }

    inline void update(const material& m, const rt::color& local_color,
        const bool update_color_materials) {

        emitted_colors =
            m.is_emissive() ?
                combine(m.get_color() * m.get_emission_intensity())
                :
                emitted_colors;
        color_materials =
            update_color_materials ?
                color_materials * local_color
                :
                color_materials;
    }
};

/** Auxiliary functions **/

#define BIAS_NORM 1.0E-3f

/* Auxiliary function that applies a bias of 1.0E-3 times the normal to the ray position,
   outward the surface contact point if outward_bias is true (so in the direction of the normal),
   inward otherwise (in the opposite direction to the normal) */
inline void apply_bias(ray& r, const rt::vector& hit_point, const rt::vector& normal,
    const bool inward, const bool outward_bias) {

    //r.set_origin(hit_point + ((inward == outward_bias) ? BIAS_NORM * normal : (-BIAS_NORM) * normal));
    r.set_origin(fma(normal, (inward == outward_bias) ? BIAS_NORM : (-BIAS_NORM), hit_point));
}


/* Auxiliary function that handles the specular reflective case */
void specular_reflective_case(ray& r, const hit& h, randomgen& rg, const real smoothness,
    const rt::vector& local_normal, const bool inward) {

    const rt::vector central_dir = get_central_reflected_direction(h, local_normal, smoothness, inward);
                    
    /* Direction according to Lambert's cosine law */
    const rt::vector dir = (smoothness >= 1.0f) ?
        central_dir
        :
        (fma(random_direction(rg, central_dir, PI), 1.0f - smoothness, central_dir)).unit();
    r.set_direction(dir);
    // Here: be careful not to go below the surface, when its local normal is almost parallel to the surface (cap the max angle to the local_normal)

    /* Apply the bias outward the surface */
    apply_bias(r, h.get_point(), h.get_normal(), inward, true);
}


/* Auxiliary function that handles the diffuse reflective case */
void diffuse_case(ray& r, const hit& h, const rt::vector& local_normal, randomgen& rg, const bool inward) {

    r.set_direction(((inward ? local_normal : (-1.0f) * local_normal) + random_direction(rg, local_normal, PI)).unit());
    // Here: be careful not to go below the surface, when its local normal is almost parallel to the surface (cap the max angle to the local_normal)

    /* Apply the bias outward the surface */
    apply_bias(r, h.get_point(), h.get_normal(), inward, true);
}


/* Auxiliary function that handles the refractive case */
void refractive_case(ray& r, const hit& h, randomgen& rg, const real scattering,
    const rt::vector& local_normal,
    const rt::vector& vx, const real sin_theta_2_sq, const bool inward,
    real& refr_index, const real next_refr_i) {

    /* Setting the refracted direction */
    r.set_direction(
        get_random_refracted_direction(
            rg,
            scattering,
            local_normal,
            vx, sin_theta_2_sq, inward
        )
    );

    /* Updating the refraction index */
    refr_index = next_refr_i;

    /* Apply the bias inward the surface */
    apply_bias(r, h.get_point(), h.get_normal(), inward, false);
}

inline rt::color background_case(const scene& scene, const ray& r,
    const accumulators& acc) {

    /* Determining the pixel of the background texture to display */
    /* Determining the spherical coordinates of the direction,
        then the UV-coordinates in the 360 image */

    // return (color_materials *
    //     (scene.background.has_texture() ?
    //         scene.background.get_color(r.get_direction())
    //         :
    //         scene.background.get_color()))
    //     + emitted_colors;
    return acc.combine(
            scene.background.has_texture() ?
                scene.background.get_color(r.get_direction())
                :
                scene.background.get_color()
            );
}


/* Path tracing function */

/*  Computes the hit of the given ray on the closest object,
    then launches one ray, in a direction and with a color depending on the surface material,
    until it is too dim, or a light-emitting object is hit, or the maximum number of bounces is reached. */

/* In recursive form, the light equation is of the form u(n) = a(n) * u(n-1) + b(n),
   in iterative form, we have an accumulator color_materials of the product of the a(k), k=n..,
   and an accumulator (emitted_colors) of the (product of a(j), j=n..k) * b(k). */

rt::color pathtrace(ray& r, scene& scene, randomgen& rg, const unsigned int bounce,
    const bool russian_roulette,
    const real init_refr_index = 1.0f) {

    accumulators acc;
        
    real refr_index = init_refr_index;
    static thread_local custom_stack<real> refr_stack(20);
    refr_stack.set_empty();

    const bool bounding_method = scene.polygons_per_bounding != 0;

    for (unsigned int i = 0; i < bounce; i++) {

        if (russian_roulette) {
            const real avg = acc.color_materials.get_average_ratio();
            if (avg < 1.0f) {
                if (rg.random_ratio() <= 1.0f - avg)
                    return acc.emitted_colors;
                else
                    acc.color_materials /= avg;
            }
        }

        const std::optional<hit> opt_h = scene.find_closest(r, bounding_method);

        if (not opt_h.has_value()) /* No object hit: background color or background texture */
            return background_case(scene, r, acc);
            
        const hit& h = opt_h.value();
        const object* obj = h.get_object();
        const material& m = scene.material_set[obj->get_material_index()];

        /* Full-intensity light source reached */
        if (m.is_emissive() && m.get_emission_intensity() >= 1.0f) {

            // if (obj->is_textured()) {
            //     const barycentric_info bary = obj->get_barycentric(h.get_point());
            //     return acc.combine(scene.sample_texture(obj->get_texture_info(), bary) * m.get_emission_intensity());
            // }
            // else {
            //     return acc.combine(m.get_emitted_color() * m.get_emission_intensity());
            // }

            return obj->is_textured() ?
                acc.combine(scene.sample_texture(obj->get_texture_info_index(), obj->get_barycentric(h.get_point())) * m.get_emission_intensity())
                :
                acc.combine(m.get_color() * m.get_emission_intensity());
        }

        /* The ray can either be transmitted (and refracted) through the surface,
            or reflected in three ways: specularly, diffusely, or in the case of total internal reflection,
            when the ray hits a surface of lower refraction index at an angle greater than the critical angle.
        */
        const real inward = h.is_inward();

        // Contains the material local color, the local normal and (soon) the reflectivity and displacement
        const map_sample ms = (obj->is_textured()) ?
            scene.sample_maps(obj->get_texture_info_index(), obj->get_barycentric(h.get_point()),
                m.get_color(), h.get_normal(), m.get_smoothness())
            :
            map_sample(m.get_color(), h.get_normal()); // reflectivity, displacement);

        const rt::vector normal = (obj->is_textured() && scene.texture_info_set[obj->get_texture_info_index()].has_normal_information()) ?
            obj->compute_normal_from_map(ms.normal_map_vector, h.get_normal())
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
                const real specular_smoothness = is_specular_bounce ? smoothness : 0.0f;
                
                specular_reflective_case(r, h, rg, specular_smoothness, normal, inward);

                /* We update color_materials only if the material reflects colors (like a christmas tree ball),
                otherwise the reflection has the original color (like a tomato) */
                acc.update(m, color, !is_specular_bounce || m.does_reflect_color());
            }
            else {

                /* Diffuse bounce */

                diffuse_case(r, h, normal, rg, inward);
                acc.update(m, color, true);
            }
        }
        else {
            /* Transmission or reflection, depending on the Fresnel coefficients Kr, Kt
                Kr is the probability that the ray is reflected, Kt the probability that the ray is transmitted */

            /* Computation of the new refraction index */
            const real next_refr_i = inward ? m.get_refraction_index() : (refr_stack.empty() ? 1.0f : refr_stack.top());
            if (inward) {
                if (refr_index != 1.0f) {
                    refr_stack.push(refr_index);
                }
            }
            else if (not refr_stack.empty()) {
                refr_stack.pop();
            }

            /* Pre-computation of the refracted direction */
            real sin_theta_2_sq;
            const rt::vector vx = get_sin_refracted(h, normal, refr_index, next_refr_i, sin_theta_2_sq);

            /* Computation of the Fresnel coefficient */
            // const real kr = inward ? h.get_fresnel(sin_theta_2_sq, refr_index, next_refr_i) : 0.0f;
            const real kr = inward ? get_schlick(h, normal, refr_index, next_refr_i) : 0.0f;

            if (inward && rg.random_ratio() * m.get_transparency() <= kr) {
            
                /* The ray is reflected */
                
                /* Is it a pure specular or a mix of specular and diffuse just like in the previous case? */
                specular_reflective_case(r, h, rg, smoothness, normal, inward);
                acc.update(m, color, false);
            }
            else {
                
                /* The ray is transmitted */

                /* Determination of whether the ray is transmitted (refracted) or in total interal reflection */
                if (sin_theta_2_sq >= 1.0f) {
                    /* Total internal reflection */

                    specular_reflective_case(r, h, rg, smoothness, normal, inward);
                    acc.update(m, color, false);
                }
                else {
                    /* Transmission */

                    refractive_case(r, h, rg, m.get_refraction_scattering(), normal,
                        vx, sin_theta_2_sq, inward, refr_index, next_refr_i);
                    acc.update(m, color, true);
                }

            }
        }
        
    }

    /* Maximum number of bounces reached: the final color is black */
    return acc.emitted_colors;
}






