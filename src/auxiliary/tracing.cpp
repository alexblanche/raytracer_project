#include <cmath>
#include "../light/headers/ray.hpp"
#include "../screen/headers/color.hpp"
#include "../scene/objects/headers/object.hpp"
#include "../scene/headers/scene.hpp"
#include "../scene/objects/headers/bounding.hpp"

// #include "headers/application.hpp"


/* ******************************************************************** */
/* *************************** Path tracing *************************** */


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

    const bool bounding_method = scene.triangles_per_bounding != 0;
    const double PI = 3.14159265358979323846;

    for (unsigned int i = 0; i < bounce; i++) {
        
        const hit h = bounding_method ? bounding::find_closest_object(r) : object::find_closest_object(r);

        if (h.object_hit()) {
            const material& m = object::set.at(h.get_obj_index())->get_material();
            const double& reflectivity = m.get_reflectivity();

            if (m.get_emission_intensity() >= 1) {
                // Light source touched
                return (color_materials * (m.get_emitted_color() * m.get_emission_intensity())) + emitted_colors;
            }
            else {
                r.set_origin(h.get_point());
                
                if (random_double(scene.rg, 1) <= m.get_specular_proba()) {
                    /* Specular bounce */

                    const rt::vector central_dir = h.get_central_direction(reflectivity);
                    
                    // Direction according to Lambert's cosine law
                    if (reflectivity >= 1) {
                        r.set_direction(central_dir);
                    }
                    else {
                        const rt::vector bouncing_dir = (central_dir +
                            ((1 - reflectivity) * h.random_reflect_single(scene.rg, central_dir, PI))).unit();
                        r.set_direction(bouncing_dir);
                    }
                    
                    const rt::color emitted_light = m.get_emitted_color() * m.get_emission_intensity();

                    // Updating the accumulators
                    emitted_colors = emitted_colors + (color_materials * emitted_light);
                    if (m.does_reflect_color()) {
                        // Reflections have the material color (like a christmas tree ball)
                        color_materials = color_materials * m.get_color();
                    }
                    // Otherwise, reflections have the original color (like a tomato)
                }
                else {
                    /* Diffuse bounce */
                    
                    const rt::vector bouncing_dir = (h.get_normal() + h.random_reflect_single(scene.rg, h.get_normal(), PI)).unit();
                    r.set_direction(bouncing_dir);
                    
                    const rt::color emitted_light = m.get_emitted_color() * m.get_emission_intensity();

                    // Updating the accumulators
                    emitted_colors = emitted_colors + (color_materials * emitted_light);
                    color_materials = color_materials * m.get_color();
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
