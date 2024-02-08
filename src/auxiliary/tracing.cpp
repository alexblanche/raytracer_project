#include "../light/headers/ray.hpp"
#include <cmath>
#include <limits>
std::numeric_limits<double> real;
const double infinity = real.infinity();
#include "../screen/headers/color.hpp"
#include "../scene/sources/headers/source.hpp"
#include "../scene/objects/headers/object.hpp"
#include "../scene/headers/scene.hpp"

#include "headers/application.hpp"


/* Tracing the ray */

/* Test raycasting function:
  Casts a ray and returns only the color of the surface hit */
rt::color raycast(const ray& r, const vector<const object*>& obj_set) {
    
    double d;
    double closest = infinity;
    int closest_index = 0;

    for (unsigned int i = 0; i < obj_set.size(); i++) {

        d = obj_set.at(i)->measure_distance(r);
        /* d is the distance between the origin of the ray and the
           intersection point with the object */
        if (d < closest) {
            closest = d;
            closest_index = i;
        }
    }

    if (closest != infinity) {
        return obj_set.at(closest_index)->get_material().get_color();
    }
    else {
        return rt::color::BLACK;
    }
}

/* Ray tracing function: computes the hit of the given ray on the closest object,
    then applies all the light from all the sources (blocked by the other objects),
    and returns the resulting color. */
rt::color raytrace(const ray& r, const vector<const object*>& obj_set, const vector<source>& light_set) {

    double d;
    double closest = infinity;
    int closest_index = 0;

    // Looking for the closest object
    for (unsigned int i = 0; i < obj_set.size(); i++) {

        d = obj_set.at(i)->measure_distance(r);
        
        /* d is the distance between the origin of the ray and the
           intersection point with the object */

        if (d < closest) {
            closest = d;
            closest_index = i;
        }
    }

    if (closest != infinity) {
        const hit h = obj_set.at(closest_index)->compute_intersection(r, closest);
        return add_col_vect(apply_lights_obj(h, obj_set, light_set));
    }
    else {
        return rt::color::BLACK; // No object hit
    }
}



/* ******************************************************************** */
/* *************************** Path tracing *************************** */


/* Computing the intersection of a ray with the scene */
hit find_closest_object(const ray& r, const vector<const object*>& obj_set, const unsigned int origin_obj_index) {
    
    double closest = infinity;
    int closest_index = -1;

    // Looking for the closest object
    for (unsigned int i = 0; i < obj_set.size(); i++) {
            
        // We do not test the intersection with the object the rays is cast from
        if (i != origin_obj_index) {
            const double d = obj_set.at(i)->measure_distance(r);
                
            /* d is the distance between the origin of the ray and the
               intersection point with the object */

            if (d < closest) {
                closest = d;
                closest_index = i;
            }
        }
    }
    if (closest_index == -1) {
        return hit();
    }
    else {
        return obj_set.at(closest_index)->compute_intersection(r, closest);
    }
}




/* Path tracing function */

/* Multiple rays version:
    Computes the hit of the given ray on the closest object,
    then recursively launches (number_of_rays) rays, with a distribution depending on the surface material,
    until either a light-emitting object is hit, or the maximum number of bounces is reached.
    The colors obtained are then combined to determine the color of the pixel. */


rt::color pathtrace_mult(const ray& r, scene& scene, const unsigned int origin_obj_index,
    const unsigned int number_of_rays, const unsigned int bounce) {

    const hit h = find_closest_object(r, scene.obj_set, origin_obj_index);

    if (h.is_hit()) {
        const material m = scene.obj_set.at(h.get_obj_index())->get_material();
        const double reflectivity = m.get_reflectivity();

        if (bounce == 0 || m.get_emission_intensity() > 0.9999) {
            // Maximum number of bounces reached or light source touched
            return m.get_emitted_color() * m.get_emission_intensity();
        }
        else if (reflectivity > 0.9999) {
            // The surface hit is a mirror
            return m.get_color() * pathtrace_mult(h.get_reflected_ray(), scene, h.get_obj_index(), number_of_rays, bounce-1);
        }
        else {
            // Angle of the cone toward which the rays are cast (pi/2 * (1-reflectivity))
            const double theta = 1.57079632679 * (1 - reflectivity);

            std::vector<rt::color> return_colors(number_of_rays);
            
            const rt::vector central_dir = h.get_central_direction(reflectivity);
            const std::vector<ray> bouncing_rays = h.random_reflect(number_of_rays, scene.rg, central_dir, theta);

            for(unsigned int i = 0; i < number_of_rays; i++) {

                const double bias = (bouncing_rays.at(i).get_direction() | central_dir) * 2;
                return_colors.at(i) = pathtrace_mult(bouncing_rays.at(i), scene, h.get_obj_index(), number_of_rays, bounce-1)
                                    * bias;
            }

            const rt::color incoming_light = average_col_vect(return_colors);
            
            return m.get_color() * incoming_light + (m.get_emitted_color() * m.get_emission_intensity());
        }
    }
    else {
        // No object hit: background color
        return scene.background;
    }
}

/* Single ray version:
    Computes the hit of the given ray on the closest object,
    then launches one ray, in a direction and with a color depending on the surface material,
    until it is too dim, or a light-emitting object is hit, or the maximum number of bounces is reached. */

/* In recursive form, the light equation is of the form u(n) = a(n) * u(n-1) + b(n),
   in iterative form, we have an accumulator color_materials of the product of the a(k), k=n..,
   and an accumulator (emitted_colors) of the (product of a(j), j=n..k) * b(k). */

rt::color pathtrace(ray& r, scene& scene, const unsigned int origin_obj_index,
    const unsigned int bounce) {

    rt::color color_materials = rt::color::WHITE;
    rt::color emitted_colors = rt::color::BLACK;

    for (unsigned int i = 0; i < bounce; i++) {

        const hit h = find_closest_object(r, scene.obj_set, origin_obj_index);

        if (h.is_hit()) {
            const material m = scene.obj_set.at(h.get_obj_index())->get_material();
            const double reflectivity = m.get_reflectivity();

            if (m.get_emission_intensity() > 0.9999) {
                // Light source touched
                return color_materials * (m.get_emitted_color() * m.get_emission_intensity()) + emitted_colors;
            }
            else {
                // Angle of the cone toward which the rays are cast (pi/2 * (1-reflectivity))
                const double theta = 1.57079632679 * (1 - reflectivity); 
                
                // Redirecting ray r
                r.set_origin(h.get_point());
                const rt::vector central_dir = h.get_central_direction(reflectivity);
                const rt::vector bouncing_dir = h.random_reflect_single(scene.rg, central_dir, theta);
                r.set_direction(bouncing_dir);
                
                const double bias = (bouncing_dir | central_dir) * 2;
                const rt::color emitted_light = m.get_emitted_color() * m.get_emission_intensity();

                // Updating the accumulators
                color_materials = color_materials * bias * m.get_color();
                emitted_colors = color_materials * emitted_light;
            }
        }
        else {
            // No object hit: background color
            return color_materials * scene.background + emitted_colors;
        }
    }

    // Maximum number of bounces reached: the final color is black
    return emitted_colors;
}
