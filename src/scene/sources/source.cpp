#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <vector>

#include "light/hit.hpp"
#include "scene/sources/source.hpp"
#include "scene/objects/sphere.hpp"
#include "scene/objects/plane.hpp"
#include "screen/color.hpp"

using namespace std;

/** The source class represents point-shaped light sources,
 * defined by their position and color.
 */

/* Constructors */

source::source(const rt::vector& p, const rt::color& c)
    : position(p), color(c) {}

source::source() {
    position = rt::vector();
    color = rt::color::WHITE;
}


/* Application of light on surfaces */


/* Applies the color of the light source on the given hit,
   or black if it is blocked by some object of the scene */
rt::color source::apply_obj(const hit& h, const vector<const object*>& obj_set) const {

    const rt::vector to_the_light = position - h.get_point();
    const double dist = to_the_light.norm();

    const ray reflected_ray(h.get_point(), to_the_light.unit());

    // Looking for an intersection with an object
    double d;

    for (unsigned int i = 0; i < obj_set.size(); i++) {
        d = obj_set.at(i)->measure_distance(reflected_ray);

        //printf("%f ", d);

        if (d > 0.1 && d <= dist) {
            // d<=dist means the light is blocked by some object
            // d==0 when the object of contact is tested
            return rt::color::BLACK;
        }
    }
    //printf("\n");

    const rt::color hit_color = h.get_object()->get_material().get_color();

    /* normal is oriented outward the object, and position - h.get_point() is oriented toward the light source,
       so cos_hit < 0 means the object is on the far side, cos_hit > 0 means the light hits the object.
       This way, we avoid doing (-cos_hit) 3 times. */
    double cos_hit = (h.get_normal() | (position - h.get_point()).unit());

    if (cos_hit < 0) {
        // The point is on the far side of the object
        return rt::color::BLACK;
    }
    else {
        const unsigned char r = (color.get_red() * hit_color.get_red()) * cos_hit / 255 ;
        const unsigned char g = (color.get_green() * hit_color.get_green()) * cos_hit / 255 ;
        const unsigned char b = (color.get_blue() * hit_color.get_blue()) * cos_hit / 255 ;
        return rt::color(r, g, b);
    }    
}
