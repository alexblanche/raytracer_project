#include <cmath>
#include "light/ray.hpp"
#include "screen/color.hpp"
#include "scene/objects/object.hpp"
// #include "scene/objects/polygon.hpp"
#include "scene/scene.hpp"
#include "scene/objects/bounding.hpp"
#include "scene/material/texture.hpp"

#include "tracing/directions.hpp" 

#include <stack>

#define PI 3.14159265358979323846f


/* ******************************************************************** */
/* *************************** Path tracing *************************** */

/** Auxiliary functions **/

/* Auxiliary function that updates the color accumulators */
void update_accumulators(
    const material& m, rt::color& emitted_colors, rt::color& color_materials,
    const rt::color& local_color,
    const bool update_color_materials) {

    if (m.is_emissive()) {
        const rt::color emitted_light = m.get_emitted_color() * m.get_emission_intensity();
        emitted_colors = emitted_colors + (color_materials * emitted_light);
    }
    
    if (update_color_materials) {
        color_materials = color_materials * local_color;
    }
}

#define BIAS_NORM 1.0E-3f

/* Auxiliary function that applies a bias of 1.0E-3 times the normal to the ray position,
   outward the surface contact point if outward_bias is true (so in the direction of the normal),
   inward otherwise (in the opposite direction to the normal) */
void apply_bias(ray& r, const rt::vector& hit_point, const rt::vector& normal,
    const bool inward, const bool outward_bias) {

    r.set_origin(hit_point + ((inward == outward_bias) ? BIAS_NORM * normal : (-BIAS_NORM) * normal));
}


/* Auxiliary function that handles the specular reflective case */
void specular_reflective_case(ray& r, const hit& h, randomgen& rg, const real reflectivity,
    const rt::vector& local_normal, const bool inward) {

    const rt::vector central_dir = get_central_reflected_direction(h, local_normal, reflectivity, inward);
                    
    /* Direction according to Lambert's cosine law */
    if (reflectivity >= 1.0f) {
        r.set_direction(central_dir);
    }
    else {
        r.set_direction(
            (central_dir +
                ((1.0f - reflectivity) * random_direction(rg, central_dir, PI))
            ).unit()
        );
    }

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

rt::color background_case(const scene& scene, const ray& r,
    const rt::color& color_materials, const rt::color& emitted_colors) {

    if (scene.background.has_texture()) {
        /* Determining the pixel of the background texture to display */
        /* Determining the spherical coordinates of the direction,
            then the UV-coordinates in the 360 image */

        const rt::vector& dir = r.get_direction();
        return (color_materials * (scene.background.get_color(dir))) + emitted_colors;

    }
    else {
        return (color_materials * scene.background.get_color()) + emitted_colors;
    }
}


/* Path tracing function */

/*  Computes the hit of the given ray on the closest object,
    then launches one ray, in a direction and with a color depending on the surface material,
    until it is too dim, or a light-emitting object is hit, or the maximum number of bounces is reached. */

/* In recursive form, the light equation is of the form u(n) = a(n) * u(n-1) + b(n),
   in iterative form, we have an accumulator color_materials of the product of the a(k), k=n..,
   and an accumulator (emitted_colors) of the (product of a(j), j=n..k) * b(k). */

rt::color pathtrace(ray& r, scene& scene, const unsigned int bounce,
    const real init_refr_index = 1.0f) {

    rt::color color_materials = rt::color::WHITE;
    rt::color emitted_colors = rt::color::BLACK;
    
    real refr_index = init_refr_index;
    std::stack<real> refr_stack;

    const bool bounding_method = scene.polygons_per_bounding != 0;


    for (unsigned int i = 0; i < bounce; i++) {

        const std::optional<hit> opt_h =
            bounding_method ?
                scene.find_closest_object_bounding(r)
                :
                scene.find_closest_object(r);

        if (opt_h.has_value()) {
            
            const hit& h = opt_h.value();
            const object* obj = h.get_object();
            const material& m = scene.material_set[obj->get_material_index()];

            auto update_acc = [&](bool reflects_colors, const rt::color& local_color) {
                update_accumulators(m, emitted_colors, color_materials, local_color, reflects_colors);
            };

            /* Full-intensity light source reached */
            if (m.is_emissive() && m.get_emission_intensity() >= 1.0f) {

                if (obj->is_textured()) {
                    // Only polygons (triangles and quads) can be textured (for now)
                    
                    const barycentric_info bary = obj->get_barycentric(h.get_point());
                    return
                        (color_materials *
                            (scene.sample_texture(obj->get_texture_info(), bary)
                                * m.get_emission_intensity())
                        )
                        + emitted_colors;
                }
                else {

                    return (color_materials * (m.get_emitted_color() * m.get_emission_intensity())) + emitted_colors;
                }                
            }

            /* The ray can either be transmitted (and refracted) through the surface,
               or reflected in three ways: specularly, diffusely, or in the case of total internal reflection,
               when the ray hits a surface of lower refraction index at an angle greater than the critical angle.
            */
            const real inward = h.is_inward();

            // Contains the material local color, the local normal and (soon) the reflectivity and displacement
            const map_sample ms = (obj->is_textured()) ?
                scene.sample_maps(obj->get_texture_info(), obj->get_barycentric(h.get_point()),
                    m.get_color(), h.get_normal(), m.get_reflectivity())
                :
                map_sample(m.get_color(), h.get_normal()); // reflectivity, displacement);

            const rt::vector normal = (obj->is_textured() && obj->get_texture_info().normal_map_index.has_value()) ?
                obj->compute_normal_from_map(ms.normal_map_vector, h.get_normal())
                :
                h.get_normal();

            const rt::color& color = ms.texture_color;
            const real reflectivity = m.get_reflectivity(); // ms.reflectivity
            //

            if (m.is_opaque()) {
                /* Diffuse or specular reflection */

                /* Testing whether the ray is reflected specularly or diffusely */
                if (m.has_specular_proba() && scene.rg.random_real(1.0f) <= m.get_specular_proba()) {
                    
                    /* Specular bounce */

                    specular_reflective_case(r, h, scene.rg, reflectivity, normal, inward);

                    /* We update color_materials only if the material reflects colors (like a christmas tree ball),
                       otherwise the reflection has the original color (like a tomato) */
                    update_acc(m.does_reflect_color(), color);
                }
                else {

                    /* Diffuse bounce */

                    diffuse_case(r, h, normal, scene.rg, inward);
                    update_acc(true, color);
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
	
                if (inward && scene.rg.random_real(1.0f) * m.get_transparency() <= kr) {
                
                    /* The ray is reflected */
                    
                    /* Is it a pure specular or a mix of specular and diffuse just like in the previous case? */
                    specular_reflective_case(r, h, scene.rg, reflectivity, normal, inward);
                    update_acc(false, color);
                }
                else {
                    
                    /* The ray is transmitted */

                    /* Determination of whether the ray is transmitted (refracted) or in total interal reflection */
                    if (sin_theta_2_sq >= 1.0f) {
                        /* Total internal reflection */

                        specular_reflective_case(r, h, scene.rg, reflectivity, normal, inward);
                        update_acc(false, color);
                    }
                    else {
                        /* Transmission */

                        refractive_case(r, h, scene.rg, m.get_refraction_scattering(), normal,
                            vx, sin_theta_2_sq, inward, refr_index, next_refr_i);
                        update_acc(true, color);
                    }

                }
            }
        }
        else {
            /* No object hit: background color or background texture */
            return background_case(scene, r, color_materials, emitted_colors);
        }
    }

    /* Maximum number of bounces reached: the final color is black */
    return emitted_colors;
}






/* ******************************************************************** */
/* Path tracing with multisample approach:
   After the first hit, multiple rays are cast */

rt::vector random_dir(scene& scene, const rt::vector central_dir, const real scattering) {
    
    return (central_dir + (1.0f - scattering) * random_direction(scene.rg, central_dir, PI)).unit();
}

rt::vector compute_bias(const rt::vector& hit_point, const rt::vector& normal,
    const bool inward, const bool outward_bias) {

    return hit_point + ((inward == outward_bias) ? (1.0E-3f)*normal : (-1.0E-3f)*normal);
}

#define FIRST true
#define SECOND false

/* Computation of the new central direction and scattering */
void compute_bouncing_ray(const material& m, const hit& h,
    // Output
    rt::vector& orig1, rt::vector& orig2, rt::vector& central_dir1, std::optional<rt::vector>& central_dir2, real& scattering1, real& scattering2,
    real& proba_1, // if scene.rg.random_real(1.0f) <= proba_1, choose central_dir1 with scattering1, otherwise central_dir2, scattering2
    rt::color& color_materials1, rt::color& color_materials2, rt::color& emitted_colors, real& init_refr_index) {

    init_refr_index = 1.0f;

    color_materials1 = rt::color::WHITE;
    color_materials2 = rt::color::WHITE;
    emitted_colors = rt::color::BLACK;

    auto update_acc = [&](bool reflects_colors, const rt::color& local_color, bool first) {
        rt::color& color_materials = (first) ? color_materials1 : color_materials2;
        update_accumulators(m, emitted_colors, color_materials, local_color, reflects_colors);
    };

    const real inward = h.is_inward();

    if (m.is_opaque()) {
        /* Diffuse or specular reflection */

        if (m.has_specular_proba()) {
            // Mix of specular and diffuse
            central_dir1 = get_central_reflected_direction(h, h.get_normal(), m.get_reflectivity(), inward);
            scattering1 = m.get_reflectivity();
            orig1 = compute_bias(h.get_point(), h.get_normal(), inward, true);
            update_acc(m.does_reflect_color(), m.get_color(), FIRST);

            proba_1 = m.get_specular_proba();

            const rt::vector& normal = h.get_normal();
            central_dir2 = inward ? normal : (-1.0f) * normal;
            scattering2 = 0.0f;
            orig2 = compute_bias(h.get_point(), normal, inward, true);
            update_acc(true, m.get_color(), SECOND);
        }
        else {
            // Only diffuse
            const rt::vector& normal = h.get_normal();
            central_dir1 = inward ? normal : (-1.0f) * normal;
            scattering1 = 0.0f;
            orig1 = compute_bias(h.get_point(), normal, inward, true);

            update_acc(true, m.get_color(), FIRST);
        }
    }
    else {
        /* Transmission or reflection, depending on the Fresnel coefficients Kr, Kt
            Kr is the probability that the ray is reflected, Kt the probability that the ray is transmitted */

        /* Computation of the new refraction index */
        const real next_refr_i = inward ? m.get_refraction_index() : 1.0f;

        /* Pre-computation of the refracted direction */
        real sin_theta_2_sq;
        const rt::vector vx = get_sin_refracted(h, h.get_normal(), 1.0f, next_refr_i, sin_theta_2_sq);

        /* Computation of the Fresnel coefficient */
        const real kr = inward ? get_schlick(h, h.get_normal(), 1.0f, next_refr_i) : 0.0f;

        if (sin_theta_2_sq >= 1.0f) {
            // Total internal reflection

            central_dir1 = get_central_reflected_direction(h, h.get_normal(), m.get_reflectivity(), inward);
            scattering1 = m.get_reflectivity();
            orig1 = compute_bias(h.get_point(), h.get_normal(), inward, true);

            update_acc(false, m.get_color(), FIRST);
        }
        else {
            // Mix of reflection and transmission

            // Transmission
            central_dir1 = get_refracted_direction(h.get_normal(), vx, sin_theta_2_sq, inward);
            scattering1 = 1.0f - m.get_refraction_scattering();

            init_refr_index = next_refr_i;
            orig1 = compute_bias(h.get_point(), h.get_normal(), inward, false);
            update_acc(true, m.get_color(), FIRST);

            if (inward) {
                // Reflection
                central_dir2 = get_central_reflected_direction(h, h.get_normal(), m.get_reflectivity(), inward);
                scattering2 = m.get_reflectivity();
                orig2 = compute_bias(h.get_point(), h.get_normal(), inward, true);

                update_acc(false, m.get_color(), SECOND);

                proba_1 = 1.0f - std::min(std::max(kr / m.get_transparency(), (real) 0.0f), (real) 1.0f);
            }
        }
    }
}

rt::color pathtrace_multisample(ray& r, scene& scene, const unsigned int bounce, const unsigned int number_of_samples) {
    
    const bool bounding_method = scene.polygons_per_bounding != 0;

    const std::optional<hit> opt_h =
        bounding_method ?
            scene.find_closest_object_bounding(r)
            :
            scene.find_closest_object(r);

    if (opt_h.has_value()) {
        const hit& h = opt_h.value();
        const object* obj = h.get_object();
        const material& m = scene.material_set[obj->get_material_index()];

        if (m.is_emissive() && m.get_emission_intensity() >= 1.0f) {

            if (obj->is_textured()) {
                // Only polygons (triangles and quads) can be textured (for now)
                
                const barycentric_info bary = obj->get_barycentric(h.get_point());
                return
                    scene.sample_texture(obj->get_texture_info(), bary)
                        * m.get_emission_intensity();
            }
            else {
                return m.get_emitted_color() * m.get_emission_intensity();
            }                
        }

        // Multiple samples cast
        
        /* Computation of the new origin, central direction and scattering */
        rt::color color_materials1;
        rt::color color_materials2;
        rt::color emitted_colors;
        rt::vector orig1;
        rt::vector orig2;
        rt::vector central_dir1;
        std::optional<rt::vector> central_dir2;
        real scattering1;
        real scattering2;
        real proba_1 = 1.0f;
        real init_refr_index;
        compute_bouncing_ray(m, h,
            orig1, orig2, central_dir1, central_dir2, scattering1, scattering2, proba_1,
            color_materials1, color_materials2, emitted_colors, init_refr_index);

        const bool one_direction = (not central_dir2.has_value()) || (proba_1 == 1.0f);

        if (one_direction) {
            // Accumulator
            rt::color output_color = rt::color::BLACK;

            for (unsigned int i = 0; i < number_of_samples; i++) {

                const rt::vector dir = random_dir(scene, central_dir1, scattering1);
                ray bouncing_ray(orig1, dir);
                const rt::color sample_color = pathtrace(bouncing_ray, scene, bounce - 1, 1.0f);
                output_color = output_color + sample_color;
            }

            return color_materials1 * (output_color / number_of_samples) + emitted_colors;
        }
        else {
            // Accumulators
            rt::color output_color1 = rt::color::BLACK;
            rt::color output_color2 = rt::color::BLACK;
            unsigned int nb_samples1 = 0;
            unsigned int nb_samples2 = 0;

            for (unsigned int i = 0; i < number_of_samples; i++) {

                const real p = scene.rg.random_real(1.0f);
                if (p <= proba_1) {
                    const rt::vector dir = random_dir(scene, central_dir1, scattering1);
                    ray bouncing_ray(orig1, dir);
                    const rt::color sample_color = pathtrace(bouncing_ray, scene, bounce - 1, init_refr_index);
                    output_color1 = output_color1 + sample_color;
                    nb_samples1++;
                }
                else {
                    const rt::vector dir = random_dir(scene, central_dir2.value(), scattering2);
                    ray bouncing_ray(orig2, dir);
                    const rt::color sample_color = pathtrace(bouncing_ray, scene, bounce - 1, 1.0f);
                    output_color2 = output_color2 + sample_color;
                    nb_samples2++;
                }
            }

            rt::color output_color = emitted_colors;
            if (nb_samples1) output_color = output_color + (color_materials1 * ((output_color1 / nb_samples1) * (((float) nb_samples1) / number_of_samples)));
            if (nb_samples2) output_color = output_color + (color_materials2 * ((output_color2 / nb_samples2) * (((float) nb_samples2) / number_of_samples)));
            return output_color;
        }
    }
    else {
        return
            (scene.background.has_texture()) ?
                scene.background.get_color(r.get_direction())
                :
                scene.background.get_color();
    }
}