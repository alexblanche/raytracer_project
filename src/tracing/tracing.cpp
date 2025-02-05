#include <cmath>
#include "light/ray.hpp"
#include "screen/color.hpp"
#include "scene/objects/object.hpp"
#include "scene/objects/polygon.hpp"
#include "scene/scene.hpp"
#include "scene/objects/bounding.hpp"
#include "scene/material/texture.hpp"
#include <stack>

#define PI 3.14159265358979323846f

/* ******************************************************************** */
/* *************************** Path tracing *************************** */

/** Auxiliary functions **/

/* Auxiliary function that updates the color accumulators */
void update_accumulators(const material& m, const object* obj, const rt::vector& hit_point,
    const std::vector<texture>& texture_set,
    rt::color& emitted_colors, rt::color& color_materials,
    const bool update_color_materials) {

    const rt::color emitted_light = m.get_emitted_color() * m.get_emission_intensity();

    /* Updating the accumulators */
    emitted_colors = emitted_colors + (color_materials * emitted_light);
    
    if (update_color_materials) {
        if (obj->is_textured()) {
            // Only polygons (triangles and quads) can be textured (for now)
            const barycentric_info bary = static_cast<const polygon*>(obj)->get_barycentric(hit_point);

            color_materials = color_materials * static_cast<const polygon*>(obj)->info.value().get_texture_color(bary, texture_set);
        }
        else {
            color_materials = color_materials * m.get_color();
        }
    }
}

/* Auxiliary function that applies a bias of 1.0E-3 times the normal to the ray position,
   outward the surface contact point if outward_bias is true (so in the direction of the normal),
   inward otherwise (in the opposite direction to the normal) */
void apply_bias(ray& r, const rt::vector& hit_point, const rt::vector& normal,
    const bool inward, const bool outward_bias) {

    r.set_origin(hit_point + ((inward == outward_bias) ? (1.0E-3f)*normal : (-1.0E-3f)*normal));
}


/* Auxiliary function that handles the specular reflective case */
void specular_reflective_case(ray& r, const hit& h, randomgen& rg, const real reflectivity, const bool inward) {

    const rt::vector central_dir = h.get_central_reflected_direction(reflectivity, inward);
                    
    /* Direction according to Lambert's cosine law */
    if (reflectivity >= 1.0f) {
        r.set_direction(central_dir);
    }
    else {
        r.set_direction(
            (central_dir +
                ((1.0f - reflectivity) * h.random_direction(rg, central_dir, PI))
            ).unit()
        );
    }

    /* Apply the bias outward the surface */
    apply_bias(r, h.get_point(), h.get_normal(), inward, true);
}


/* Auxiliary function that handles the diffuse reflective case */
void diffuse_case(ray& r, const hit& h, randomgen& rg, const bool inward) {

    const rt::vector& normal = h.get_normal();
    r.set_direction(((inward ? normal : (-1.0f) * normal) + h.random_direction(rg, normal, PI)).unit());

    /* Apply the bias outward the surface */
    apply_bias(r, h.get_point(), normal, inward, true);
}


/* Auxiliary function that handles the refractive case */
void refractive_case(ray& r, const hit& h, randomgen& rg, const real scattering,
    const rt::vector& vx, const real sin_theta_2_sq, const bool inward,
    real& refr_index, const real next_refr_i) {

    /* Setting the refracted direction */
    r.set_direction(
        h.get_random_refracted_direction(
            rg,
            scattering,
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
        return (color_materials * (scene.background.get_color(dir) /* * ((real) 1.3f) */)) + emitted_colors; // TEMPORARY

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
    if (refr_index != 1.0f) {
        refr_stack.push(1.0f);
    }

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

            auto update_acc = [&](bool reflects_colors) {
                update_accumulators(m, obj, h.get_point(), scene.texture_set, emitted_colors, color_materials, reflects_colors);
            };


            /* Full-intensity light source reached */
            if (m.get_emission_intensity() >= 1.0f) {

                if (obj->is_textured()) {
                    // Only polygons (triangles and quads) can be textured (for now)
                    
                    const barycentric_info bary = static_cast<const polygon*>(obj)->get_barycentric(h.get_point());
                    return
                        (color_materials *
                            (static_cast<const polygon*>(obj)->info.value().get_texture_color(bary, scene.texture_set)
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

            if (m.get_transparency() == 0.0f) {
                /* Diffuse or specular reflection */

                /* Testing whether the ray is reflected specularly or diffusely */
                if (m.get_specular_proba() != 0.0f && scene.rg.random_real(1.0f) <= m.get_specular_proba()) {
                    
                    /* Specular bounce */

                    specular_reflective_case(r, h, scene.rg, m.get_reflectivity(), inward);

                    /* We update color_materials only if the material reflects colors (like a christmas tree ball),
                       otherwise the reflection has the original color (like a tomato) */
                    update_acc(m.does_reflect_color());
                }
                else {

                    /* Diffuse bounce */

                    diffuse_case(r, h, scene.rg, inward);
                    update_acc(true);
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
                const rt::vector vx = h.get_sin_refracted(refr_index, next_refr_i, sin_theta_2_sq);

                /* Computation of the Fresnel coefficient */
                // const real kr = inward ? h.get_fresnel(sin_theta_2_sq, refr_index, next_refr_i) : 0.0f;
                const real kr = inward ? h.get_schlick(refr_index, next_refr_i) : 0.0f;
	
                if (inward && scene.rg.random_real(1.0f) * m.get_transparency() <= kr) {
                
                    /* The ray is reflected */
                    
                    /* Is it a pure specular or a mix of specular and diffuse just like in the previous case? */
                    specular_reflective_case(r, h, scene.rg, m.get_reflectivity(), inward);
                    update_acc(false);
                }
                else {
                    
                    /* The ray is transmitted */

                    /* Determination of whether the ray is transmitted (refracted) or in total interal reflection */
                    if (sin_theta_2_sq >= 1.0f) {
                        /* Total internal reflection */

                        specular_reflective_case(r, h, scene.rg, m.get_reflectivity(), inward);
                        update_acc(false);
                    }
                    else {
                        /* Transmission */

                        refractive_case(r, h, scene.rg, m.get_refraction_scattering(),
                            vx, sin_theta_2_sq, inward, refr_index, next_refr_i);
                        update_acc(true);
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

rt::vector random_dir(const hit& h, scene& scene, const rt::vector central_dir, const real scattering) {
    
    return (central_dir + (1.0f - scattering) * h.random_direction(scene.rg, central_dir, PI)).unit();
}

/* Computation of the new central direction and scattering */
void compute_bouncing_ray(ray& r, const material& m, const hit& h, const object* obj, scene& scene,
    // Output
    rt::vector& central_dir, real& scattering, rt::color& color_materials, rt::color& emitted_colors, real& init_refr_index) {

    init_refr_index = 1.0f;

    color_materials = rt::color::WHITE;
    emitted_colors = rt::color::BLACK;

    auto update_acc = [&](bool reflects_colors) {
        update_accumulators(m, obj, h.get_point(), scene.texture_set, emitted_colors, color_materials, reflects_colors);
    };

    const real inward = h.is_inward();

    if (m.get_transparency() == 0.0f) {
        /* Diffuse or specular reflection */

        /* Testing whether the ray is reflected specularly or diffusely */
        if (m.get_specular_proba() != 0.0f && scene.rg.random_real(1.0f) <= m.get_specular_proba()) {
            
            /* Specular bounce */

            central_dir = h.get_central_reflected_direction(m.get_reflectivity(), inward);
            scattering = m.get_reflectivity();
            apply_bias(r, h.get_point(), h.get_normal(), inward, true);

            /* We update color_materials only if the material reflects colors (like a christmas tree ball),
                otherwise the reflection has the original color (like a tomato) */
            update_acc(m.does_reflect_color());
        }
        else {

            /* Diffuse bounce */

            const rt::vector& normal = h.get_normal();
            central_dir = inward ? normal : (-1.0f) * normal;
            scattering = 0.0f;
            apply_bias(r, h.get_point(), normal, inward, true);

            update_acc(true);
        }
    }
    else {
        /* Transmission or reflection, depending on the Fresnel coefficients Kr, Kt
            Kr is the probability that the ray is reflected, Kt the probability that the ray is transmitted */

        /* Computation of the new refraction index */
        const real next_refr_i = inward ? m.get_refraction_index() : 1.0f;

        /* Pre-computation of the refracted direction */
        real sin_theta_2_sq;
        const rt::vector vx = h.get_sin_refracted(1.0f, next_refr_i, sin_theta_2_sq);

        /* Computation of the Fresnel coefficient */
        const real kr = inward ? h.get_schlick(1.0f, next_refr_i) : 0.0f;

        if (inward && scene.rg.random_real(1.0f) * m.get_transparency() <= kr) {
        
            /* The ray is reflected */
            
            central_dir = h.get_central_reflected_direction(m.get_reflectivity(), inward);
            scattering = m.get_reflectivity();
            apply_bias(r, h.get_point(), h.get_normal(), inward, true);

            update_acc(false);
        }
        else {
            
            /* The ray is transmitted */

            /* Determination of whether the ray is transmitted (refracted) or in total interal reflection */
            if (sin_theta_2_sq >= 1.0f) {
                /* Total internal reflection */

                central_dir = h.get_central_reflected_direction(m.get_reflectivity(), inward);
                scattering = m.get_reflectivity();
                apply_bias(r, h.get_point(), h.get_normal(), inward, true);

                update_acc(false);
            }
            else {
                /* Transmission */

                /* Setting the refracted direction */
                central_dir = h.get_refracted_direction(vx, sin_theta_2_sq, inward);
                scattering = m.get_refraction_scattering() * 1.57079632679f;

                init_refr_index = next_refr_i;
                apply_bias(r, h.get_point(), h.get_normal(), inward, false);

                update_acc(true);
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

        if (m.get_emission_intensity() >= 1.0f) {

            if (obj->is_textured()) {
                // Only polygons (triangles and quads) can be textured (for now)
                
                const barycentric_info bary = static_cast<const polygon*>(obj)->get_barycentric(h.get_point());
                return
                    static_cast<const polygon*>(obj)->info.value().get_texture_color(bary, scene.texture_set)
                        * m.get_emission_intensity();
            }
            else {
                return m.get_emitted_color() * m.get_emission_intensity();
            }                
        }

        // Multiple samples cast
        
        /* Computation of the new origin, central direction and scattering */
        rt::color color_materials;
        rt::color emitted_colors;
        rt::vector central_dir;
        real scattering;
        real init_refr_index;
        compute_bouncing_ray(r, m, h, obj, scene, central_dir, scattering, color_materials, emitted_colors, init_refr_index);
        
        // Accumulator
        rt::color output_color = rt::color::BLACK;

        for (unsigned int sample = 0; sample < number_of_samples; sample++) {

            const rt::vector dir = random_dir(h, scene, central_dir, scattering);
            ray bouncing_ray(r.get_origin(), dir);
            const rt::color sample_color = pathtrace(bouncing_ray, scene, bounce - 1, init_refr_index);
            output_color = output_color + sample_color;
        }

        return color_materials * (output_color / number_of_samples) + emitted_colors;
    }
    else {
        return
            (scene.background.has_texture()) ?
                scene.background.get_color(r.get_direction())
                :
                scene.background.get_color();
    }
}