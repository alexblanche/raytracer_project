#include <vector>

#include "legacy/source.hpp"
#include "legacy/objects/sphere.hpp"
#include "legacy/objects/plane.hpp"

#include <iostream>

/** The source class represents point-shaped light sources,
 * defined by their position and color.
 */

/* Constructors */

source::source()
    : position(rt::vector()), color(rt::color(255, 255, 255)) {}

source::source(const rt::vector& position, const rt::color& color)
    : position(position), color(color) {}



/* Application of light on surfaces */


/* Applies the color of the light source on the given hit,
   or black if it is blocked by some object of the scene */
rt::color source::apply_obj(const hit& h, const std::vector<const object*>& obj_set) const {

    const rt::vector to_the_light = position - h.get_point();
    const double dist = to_the_light.norm();

    const ray reflected_ray(h.get_point(), to_the_light.unit());

    // Looking for an intersection with an object
    for (const object* obj : obj_set) {
        const double d = obj->measure_distance(reflected_ray);

        if (d > 0.1 && d <= dist) {
            // d<=dist means the light is blocked by some object
            // d==0 when the object of contact is tested
            return rt::color::BLACK;
        }
    }

    const rt::color hit_color = h.get_object()->get_color();

    /* normal is oriented outward the object, and position - h.get_point() is oriented toward the light source,
       so cos_hit < 0 means the object is on the far side, cos_hit > 0 means the light hits the object. */
    const double cos_hit = (h.get_normal() | (position - h.get_point()).unit());

    if (cos_hit < 0) {
        // The point is on the far side of the object
        return rt::color::BLACK;
    }
    else {
        const double r = (color.get_red()   * hit_color.get_red())   * cos_hit / 255 ;
        const double g = (color.get_green() * hit_color.get_green()) * cos_hit / 255 ;
        const double b = (color.get_blue()  * hit_color.get_blue())  * cos_hit / 255 ;
        return rt::color(r, g, b);
    }    
}
