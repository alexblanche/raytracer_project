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


rt::color pathtrace(const ray& r, const vector<const object*>& obj_set, const unsigned int origin_obj_index,
    const unsigned int number_of_rays, const unsigned int bounce, randomgen& rg) {

    double d;
    double closest = infinity;
    int closest_index = 0;

    // Looking for the closest object
    for (unsigned int i = 0; i < obj_set.size(); i++) {
        
        // We do not test the intersection with the object the rays is cast from
        if (i != origin_obj_index) {
            d = obj_set.at(i)->measure_distance(r);
            
            /* d is the distance between the origin of the ray and the
            intersection point with the object */

            if (d < closest) {
                closest = d;
                closest_index = i;
            }
        }
    }

    if (closest != infinity) {
        const hit h = obj_set.at(closest_index)->compute_intersection(r, closest);
        const material m = obj_set.at(h.get_obj_index())->get_material();
        const double reflectivity = m.get_reflectivity();

        if (bounce == 0 || m.get_emission_intensity() > 0.9999) {
            // Maximum number of bounces reached or light source touched
            //rt::vector dir = r.get_direction();
            //rt::vector orig = r.get_origin();
            //printf("Ray origin : (%f, %f, %f), dir : (%f, %f, %f), ", orig.x, orig.y, orig.z, dir.x, dir.y, dir.z);
            //printf("bounce = %u, emission = %f \n", bounce, m.get_emission_intensity());

            return m.get_emitted_color() * m.get_emission_intensity();
        }
        else if (reflectivity > 0.9999) {
            // The surface hit is a mirror
            return m.get_color() * pathtrace(h.reflect_ray(), obj_set, h.get_obj_index(), number_of_rays, bounce-1, rg);
        }
        else {
            /* Determination of the disk toward which the bounced rays are cast*/

            // Angle between the direction vector and the extremity of the disk (pi/2 * (1-reflectivity))
            const double theta = 1.57079632679 * (1 - reflectivity);
            rt::vector normal = h.get_normal();

            std::vector<rt::color> return_colors(number_of_rays);

            /*
            const double piovertwo = 1.57079632679;
            const unsigned int number_of_reflected = reflectivity * number_of_rays;

            if (number_of_reflected != 0) {
                const std::vector<ray> reflected_rays = h.random_reflect(number_of_reflected, rg, 1, acos(-(r.get_direction() | normal)));
                for(unsigned int i = 0; i < number_of_reflected; i++) {

                    return_colors.at(i) = pathtrace(reflected_rays.at(i), obj_set, h.get_obj_index(), number_of_rays, bounce-1, rg)
                                        * ((reflected_rays.at(i).get_direction() | normal) * 2);
                }
            }
            if (number_of_reflected != number_of_rays) {
                const std::vector<ray> diffuse_rays = h.random_reflect(number_of_rays - number_of_reflected, rg, 0, piovertwo);
                for(unsigned int i = number_of_reflected; i < number_of_rays; i++) {

                    return_colors.at(i) = pathtrace(diffuse_rays.at(i - number_of_reflected), obj_set, h.get_obj_index(), number_of_rays, bounce-1, rg)
                                        * ((diffuse_rays.at(i - number_of_reflected).get_direction() | normal) * 2);
                }
            }           
            */
            const std::vector<ray> bouncing_rays = h.random_reflect(number_of_rays, rg, reflectivity, theta);
                for(unsigned int i = 0; i < number_of_rays; i++) {

                    return_colors.at(i) = pathtrace(bouncing_rays.at(i), obj_set, h.get_obj_index(), number_of_rays, bounce-1, rg)
                                        * ((bouncing_rays.at(i).get_direction() | normal) * 2);
                }

            const rt::color incoming_light = average_col_vect(return_colors);

            return m.get_color() * incoming_light + (m.get_emitted_color() * m.get_emission_intensity());
        }
    }
    else {
        // No object hit: background color
        return rt::color(255, 255, 255); 
    }
}
