#include <cmath>
#include "light/ray.hpp"
#include "screen/color.hpp"
#include "scene/objects/object.hpp"
#include "scene/scene.hpp"
#include "scene/objects/bounding.hpp"
#include "scene/material/texture.hpp"


/* ******************************************************************** */
/* *************************** Path tracing *************************** */

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

    const bool bounding_method = scene.triangles_per_bounding != 0;
    const double PI = 3.14159265358979323846;

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
               or reflected in two ways: specularly or diffusely */
            const rt::vector hit_point = h.get_point();
            const rt::vector normal = h.get_normal();
            const double inward = (r.get_direction() | normal) <= 0;

            /* Testing whether the ray is transmitted or reflected on the surface */
            if (random_double(scene.rg, 1) <= m.get_transparency()) {

                /* Transmission */

                /* Setting the position with a bias inward the surface */
                r.set_origin(hit_point + (inward ? (-0.001)*normal : 0.001*normal));

                /* Setting the refracted direction */
                r.set_direction(h.get_random_refracted_direction(
                    scene.rg,
                    refr_index, m.get_refraction_index(),
                    m.get_refraction_scattering()));

                /* Updating the refraction index */
                refr_index = inward ? m.get_refraction_index() : 1;
                
                update_accumulators(m, obj, hit_point, scene.texture_set, emitted_colors, color_materials, true);

            }
            else {

                /* Reflection */

                /* Setting the position with a bias outward the surface */
                r.set_origin(hit_point + (inward ? 0.001*normal : (-0.001)*normal));

                /* Testing whether the ray is reflected specularly or diffusely */
                if (random_double(scene.rg, 1) <= m.get_specular_proba()) {
                    
                    /* Specular bounce */

                    const double reflectivity = m.get_reflectivity();
                    const rt::vector central_dir = h.get_central_reflected_direction(reflectivity, inward);
                    
                    // Direction according to Lambert's cosine law
                    if (reflectivity >= 1) {
                        r.set_direction(central_dir);
                    }
                    else {
                        r.set_direction((central_dir +
                            ((1 - reflectivity) * h.random_direction(scene.rg, central_dir, PI))).unit());
                    }

                    /* We update color_materials only if the material reflects colors (like a christmas tree ball),
                       otherwise the reflection has the original color (like a tomato) */
                    update_accumulators(m, obj, hit_point, scene.texture_set, emitted_colors, color_materials, m.does_reflect_color());
                }
                else {
                    /* Diffuse bounce */
                    r.set_direction(((inward ? normal : (-1) * normal) + h.random_direction(scene.rg, normal, PI)).unit());
                    update_accumulators(m, obj, hit_point, scene.texture_set, emitted_colors, color_materials, true);
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
