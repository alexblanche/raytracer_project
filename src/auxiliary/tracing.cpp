#include "../light/headers/ray.hpp"
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

/*
rt::color pathtrace(const ray& r, const vector<const object*>& obj_set, const unsigned int number_of_rays, const unsigned int bounce) {

    double d;
    double closest = infinity;
    int closest_index = 0;

    // Looking for the closest object
    for (unsigned int i = 0; i < obj_set.size(); i++) {

        d = obj_set.at(i)->measure_distance(r);
        
        * d is the distance between the origin of the ray and the
           intersection point with the object *

        if (d < closest) {
            closest = d;
            closest_index = i;
        }
    }

    if (closest != infinity && bounce != 0) {

        const hit h = obj_set.at(closest_index)->compute_intersection(r, closest);
        const std::vector<ray> rays = h.random_reflect(number_of_rays, 1, );
        
        return add_col_vect();
    }
    else {
        return rt::color::BLACK; // No object hit or maximum number of bounces reached
    }
}
*/