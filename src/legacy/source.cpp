#include <vector>

#include "legacy/source.hpp"
#include "legacy/objects/sphere.hpp"
#include "legacy/objects/plane.hpp"

#include <iostream>

#include <algorithm>
#include <optional>

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

    // Testing for an intersection with an object
    
    auto it =
        std::find_if(obj_set.begin(), obj_set.end(),
            [&reflected_ray, &dist](const object* obj) {
                const std::optional<double> d = obj->measure_distance(reflected_ray);
                return d.has_value()
                    && d.value() < dist
                    && d.value() > 0.001;
            });
    if (it != obj_set.end()) {
        return rt::color::BLACK;
    }

    const rt::color& hit_color = h.get_object()->get_color();

    /* normal is oriented outward the object, and position - h.get_point() is oriented toward the light source,
       so cos_hit < 0 means the object is on the far side, cos_hit > 0 means the light hits the object. */
    const double cos_hit = (h.get_normal() | (position - h.get_point()).unit());

    if (cos_hit < 0) {
        // The point is on the far side of the object
        return rt::color::BLACK;
    }
    else {
        const double r = (color.get_red()   * hit_color.get_red())   * cos_hit / 255.0 ;
        const double g = (color.get_green() * hit_color.get_green()) * cos_hit / 255.0 ;
        const double b = (color.get_blue()  * hit_color.get_blue())  * cos_hit / 255.0 ;
        return rt::color(r, g, b);
    }    
}
