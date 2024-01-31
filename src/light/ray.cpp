#include "headers/ray.hpp"
#include <limits>
std::numeric_limits<double> real;
const double infinity = real.infinity();
#include "../screen/headers/color.hpp"
#include "headers/source.hpp"

using namespace std;

/** 
 * The ray class contains the information of a ray of light:
 * the point of origin, the direction vector and the color.
*/

/* Constructors */

ray::ray(const rt::vector& o, const rt::vector& d, const rt::color& c)
    : origin(o), direction(d), color(c) {}

ray::ray(const rt::vector& o, const rt::vector& d)
    : origin(o), direction(d), color(rt::color::WHITE) {}


ray::ray() {
    origin = rt::vector();
    direction = rt::vector();
    color = rt::color();
}

/* Accessors */

rt::vector ray::get_origin() const {
    return origin;
}

rt::vector ray::get_direction() const {
    return direction;
}

rt::color ray::get_color() const {
    return color;
}


/* Tracing the ray */

/* Test raycasting function:
  Casts a ray and returns only the color of the surface hit */
rt::color ray::cast_ray(const vector<object>& obj_set) const {
    
    double d;
    double closest = infinity;
    int closest_index = 0;

    for (unsigned int i = 0; i < obj_set.size(); i++) {
        d = obj_set.at(i).send(*this);
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
        return obj_set.at(closest_index).get_color();
    }
    else {
        return rt::color::BLACK;
    }
}

/* Ray tracing function: computes the hit of the given ray on the closest object,
    then applies all the light from all the sources (blocked by the other objects),
    and returns the resulting color. */
rt::color ray::launch_ray(const vector<object>& obj_set, const vector<source>& light_set) const {

    double d;
    double closest = infinity;
    int closest_index = 0;

    // Seeking for the closest object
    for (unsigned int i = 0; i < obj_set.size(); i++) {

        d = obj_set.at(i).send(*this);
        if (d < closest) {
            closest = d;
            closest_index = i;
        }
    }

    if (closest == infinity) {
        return rt::color::BLACK; // No sphere hit, Black is returned as the color of the 'vacuum'
    }
    else {
        const hit h = obj_set.at(closest_index).intersect(*this, closest);

        //return add_col_vect (apply_lights(h, light_set));
        return add_col_vect (apply_lights_obj(h, obj_set, light_set));
    }
}