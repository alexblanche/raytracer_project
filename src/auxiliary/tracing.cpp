#include "../light/headers/ray.hpp"
#include <limits>
std::numeric_limits<double> real;
const double infinity = real.infinity();
#include "../screen/headers/color.hpp"
#include "../light/headers/source.hpp"
#include "../objects/headers/object.hpp"
#include "headers/application.hpp"


/* Tracing the ray */

/* Test raycasting function:
  Casts a ray and returns only the color of the surface hit */
rt::color cast_ray(const ray& r, const vector<const object*>& obj_set) {
    
    double d;
    double closest = infinity;
    int closest_index = 0;

    for (unsigned int i = 0; i < obj_set.size(); i++) {
        d = obj_set.at(i)->send(r);
        /*
            d is the distance between the origin of the ray and the
            intersection point with the object
        */
        if (d < closest) {
            closest = d;
            closest_index = i;
        }
    }

    if (closest != infinity) {
        return obj_set.at(closest_index)->get_color();
    }
    else {
        return rt::color::BLACK;
    }
}

/* Ray tracing function: computes the hit of the given ray on the closest object,
    then applies all the light from all the sources (blocked by the other objects),
    and returns the resulting color. */
rt::color launch_ray(const ray& r, const vector<const object*>& obj_set, const vector<source>& light_set) {

    double d;
    double closest = infinity;
    int closest_index = 0;

    // Seeking for the closest object
    for (unsigned int i = 0; i < obj_set.size(); i++) {

        d = obj_set.at(i)->send(r);
        if (d < closest) {
            closest = d;
            closest_index = i;
        }
    }

    if (closest == infinity) {
        return rt::color::BLACK; // No sphere hit, Black is returned as the color of the 'vacuum'
    }
    else {
        const hit h = obj_set.at(closest_index)->intersect(r, closest);

        // return add_col_vect(apply_lights(h, light_set));
        return add_col_vect(apply_lights_obj(h, obj_set, light_set));
    }
}