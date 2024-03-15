#include <cmath>
#include "light/ray.hpp"
#include "screen/color.hpp"
#include "scene/objects/object.hpp"
#include "scene/scene.hpp"
#include "scene/objects/bounding.hpp"
#include "scene/material/texture.hpp"
#include <stack>

#define PI 3.14159265358979323846


/* ******************************************************************** */
/* *************************** Path tracing *************************** */

/** Auxiliary functions **/

/* Auxiliary function that updates the color accumulators */
void update_accumulators(const material& m, const object*& obj, const rt::vector& hit_point,
    std::vector<const texture*>& texture_set,
    rt::color& emitted_colors, rt::color& color_materials,
    const bool update_color_materials) {

    const rt::color emitted_light = m.get_emitted_color() * m.get_emission_intensity();

    /* Updating the accumulators */
    emitted_colors = emitted_colors + (color_materials * emitted_light);
    
    if (update_color_materials) {
        if (m.is_textured()) {
            // Only triangles and quads can be textured (for now)
            double l1, l2;
            const bool lower_triangle = obj->get_barycentric(hit_point, l1, l2);

            color_materials = color_materials * m.get_texture_color(l1, l2, lower_triangle, texture_set);
        }
        else {
            color_materials = color_materials * m.get_color();
        }
    }
}

/* Auxiliary function that applies a bias of 0.001 times the normal to the ray position,
   outward the surface contact point if outward_bias is true (so in the direction of the normal),
   inward otherwise (in the opposite direction to the normal) */
void apply_bias(ray& r, const rt::vector& hit_point, const rt::vector& normal,
    const bool inward, const bool outward_bias) {

    if (outward_bias) {
        r.set_origin(hit_point + (inward ? 0.001*normal : (-0.001)*normal));
    }
    else {
        r.set_origin(hit_point + (inward ? (-0.001)*normal : 0.001*normal));
    }
}


/* Auxiliary function that handles the specular reflective case */
void specular_reflective_case(ray& r, const hit& h, randomgen& rg, const double& reflectivity, const bool inward) {

    const rt::vector central_dir = h.get_central_reflected_direction(reflectivity, inward);
                    
    // Direction according to Lambert's cosine law
    if (reflectivity >= 1) {
        r.set_direction(central_dir);
    }
    else {
        r.set_direction(
            (central_dir +
                ((1 - reflectivity) * h.random_direction(rg, central_dir, PI))
            ).unit()
        );
    }

    /* Apply the bias outward the surface */
    apply_bias(r, h.get_point(), h.get_normal(), inward, true);
}


/* Auxiliary function that handles the diffuse reflective case */
void diffuse_case(ray& r, const hit& h, randomgen& rg, const bool inward) {

    const rt::vector normal = h.get_normal();
    r.set_direction(((inward ? normal : (-1) * normal) + h.random_direction(rg, normal, PI)).unit());

    /* Apply the bias outward the surface */
    apply_bias(r, h.get_point(), normal, inward, true);
}


/* Auxiliary function that handles the refractive case */
void refractive_case(ray& r, const hit& h, randomgen& rg, const double& scattering,
    const rt::vector& vx, const double& sin_theta_2_sq, const bool inward,
    double& refr_index, const double& next_refr_i) {

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

#define UPDATE_ACC(reflects_colors) update_accumulators(m, obj, h.get_point(), scene.texture_set, emitted_colors, color_materials, reflects_colors)


/* Path tracing function */

/*  Computes the hit of the given ray on the closest object,
    then launches one ray, in a direction and with a color depending on the surface material,
    until it is too dim, or a light-emitting object is hit, or the maximum number of bounces is reached. */

/* In recursive form, the light equation is of the form u(n) = a(n) * u(n-1) + b(n),
   in iterative form, we have an accumulator color_materials of the product of the a(k), k=n..,
   and an accumulator (emitted_colors) of the (product of a(j), j=n..k) * b(k). */

rt::color pathtrace(ray& r, scene& scene, const unsigned int bounce) {

    rt::color color_materials = rt::color::WHITE;
    rt::color emitted_colors = rt::color::BLACK;
    
    double refr_index = 1;
    std::stack<double> refr_stack;

    const bool bounding_method = scene.triangles_per_bounding != 0;


    for (unsigned int i = 0; i < bounce; i++) {
        
        const hit h = bounding_method ? scene.find_closest_object_bounding(r) : scene.find_closest_object(r);

        if (h.object_hit()) {
            const object* obj = h.get_object();
            const material m = obj->get_material();

            /* Full-intensity light source reached */
            if (m.get_emission_intensity() >= 1) {
                return (color_materials * (m.get_emitted_color() * m.get_emission_intensity())) + emitted_colors;
            }

            /* The ray can either be transmitted (and refracted) through the surface,
               or reflected in three ways: specularly, diffusely, or in the case of total internal reflection,
               when the ray hits a surface of lower refraction index at an angle greater than the critical angle.
            */
            const double inward = (r.get_direction() | h.get_normal()) <= 0;

            if (m.get_transparency() == 0) {
                /* Diffuse or specular reflection */

                /* Testing whether the ray is reflected specularly or diffusely */
                if (random_double(scene.rg, 1) <= m.get_specular_proba()) {
                    
                    /* Specular bounce */

                    specular_reflective_case(r, h, scene.rg, m.get_reflectivity(), inward);

                    /* We update color_materials only if the material reflects colors (like a christmas tree ball),
                       otherwise the reflection has the original color (like a tomato) */
                    UPDATE_ACC(m.does_reflect_color());
                }
                else {

                    /* Diffuse bounce */

                    diffuse_case(r, h, scene.rg, inward);
                    UPDATE_ACC(true);
                }
            }
            else {
                /* Transmission or reflection, depending on the Fresnel coefficients Kr, Kt
	               Kr is the probability that the ray is reflected, Kt the probability that the ray is transmitted */

                /* Computation of the new refraction index */
                const double next_refr_i = inward ? m.get_refraction_index() : (refr_stack.empty() ? 1 : refr_stack.top());
                if (inward) {
                    refr_stack.push(next_refr_i);
                }
                else if (not refr_stack.empty()) {
                    refr_stack.pop();
                }

                /* Pre-computation of the refracted direction */
                double sin_theta_2_sq;
                const rt::vector vx = h.get_sin_refracted(refr_index, next_refr_i, sin_theta_2_sq);

                /* Computation of the Fresnel coefficient */
                // const double kr = inward ? h.get_fresnel(sin_theta_2_sq, refr_index, next_refr_i) : 0;
                const double kr = inward ? h.get_schlick(refr_index, next_refr_i) : 0;
	
                if (inward && random_double(scene.rg, 1) * m.get_transparency() <= kr) {
                
                    /* The ray is reflected */
                    
                    /* Is it a pure specular or a mix of specular and diffuse just like in the previous case? */
                    specular_reflective_case(r, h, scene.rg, m.get_reflectivity(), inward);
                    UPDATE_ACC(false);
                }
                else {
                    
                    /* The ray is transmitted */

                    /* Determination of whether the ray is transmitted (refracted) or in total interal reflection */
                    if (sin_theta_2_sq >= 1) {
                        /* Total internal reflection */

                        specular_reflective_case(r, h, scene.rg, m.get_reflectivity(), inward);
                        UPDATE_ACC(false);
                    }
                    else {
                        /* Transmission */

                        refractive_case(r, h, scene.rg, m.get_refraction_scattering(),
                            vx, sin_theta_2_sq, inward, refr_index, next_refr_i);
                        UPDATE_ACC(true);
                    }

                }
            }
        }
        else {
            // No object hit: background color
            return (color_materials * scene.background) + emitted_colors;
        }
    }

    // Maximum number of bounces reached: the final color is black
    return emitted_colors;
}
