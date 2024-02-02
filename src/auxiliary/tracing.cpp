#include "../light/headers/ray.hpp"
#include <cmath>
#include <limits>
std::numeric_limits<double> real;
const double infinity = real.infinity();
#include "../screen/headers/color.hpp"
#include "../scene/sources/headers/source.hpp"
#include "../scene/objects/headers/object.hpp"
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

/* Path tracing function: computes the hit of the given ray on the closest object,
    then recursively launches (number_of_rays) rays, with a distribution depending on the surface material,
    until either a light-emitting object is hit, or the maximum number of bounces is reached.
    The colors obtained are then combined to determine the color of the pixel. */


rt::color pathtrace(const ray& r, const vector<const object*>& obj_set,
    const unsigned int number_of_rays, const unsigned int bounce) {

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
        material m = obj_set.at(h.get_obj_index())->get_material();

        if (bounce == 0 || m.get_emission_intensity() == 1) {
            // Maximum number of bounces reached or light source touched
            return m.get_emitted_color() * m.get_emission_intensity();
        }
        else if (m.get_reflectivity() == 1) {
            // The surface hit is a mirror
            return pathtrace(h.reflect_ray(), obj_set, number_of_rays, bounce-1);
        }
        else {
            /* Determination of the disk toward which the bounced rays are cast*/

            // Angle between the direction vector and the extremity of the disk (pi/2 * reflectivity)
            const double theta = 1.57079632679 * m.get_reflectivity();
            const double sintheta = sin(theta);
            const double dist = sqrt((1/(sintheta * sintheta)) - 1);

            const std::vector<ray> bouncing_rays = h.random_reflect(number_of_rays, 1, dist);
            std::vector<rt::color> return_colors(number_of_rays);

            for(unsigned int i = 0; i < number_of_rays; i++) {
                return_colors.at(i) = pathtrace(bouncing_rays.at(i), obj_set, number_of_rays, bounce-1);
            }

            const rt::color incoming_light = add_col_vect(return_colors);

            return (incoming_light * m.get_color()) + (m.get_emitted_color() * m.get_emission_intensity());
        }
    }
    else {
        // No object hit
        return rt::color::BLACK; 
    }
}
