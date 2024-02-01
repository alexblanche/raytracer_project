#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <vector>
#include "headers/hit.hpp"
#include "headers/source.hpp"
#include "../objects/headers/sphere.hpp"
#include "../objects/headers/plane.hpp"
#include "../screen/headers/color.hpp"

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

/* Accessors */

rt::vector source::get_position() const {
    return position;
}

rt::color source::get_color() const {
    return color;
}


/* Application of light on surfaces */

/* Applies the color of the light source on the given hit,
   while not taking into account the objects of the scene */
/*
rt::color source::apply(const hit& h) const {

    const rt::color hit_color = h.get_color();
    const double cos_hit = ((h.get_normal()).unit() | (h.get_point() - position).unit());
    if (cos_hit < 0) {
        // The point is on the far side of the sphere
        return rt::color::BLACK;
    } 
    else {
        const unsigned char r = (color.get_red() * hit_color.get_red()) * cos_hit / 255 ;
        const unsigned char g = (color.get_green() * hit_color.get_green()) * cos_hit / 255 ;
        const unsigned char b = (color.get_blue() * hit_color.get_blue()) * cos_hit / 255 ;
        * Application of a light on a surface
        The formula is:
        (r1,g1,b1) -> (r2,g2,b2) = (r1*r2/255, g1*g2/255, b1*b2/255) *
        return rt::color(r, g, b);
    }
}
*/

/* Applies the color of the light source on the given hit,
   or black if it is blocked by some object of the scene */
rt::color source::apply_obj(const hit& h, const vector<const object*>& obj_set) const {

    const rt::vector to_the_light = position - h.get_point();
    const double dist = to_the_light.norm();

    const ray reflected_ray(h.get_point(), to_the_light.unit(), rt::color::WHITE);

    // Looking for an intersection with a sphere or a plane
    double d;

    for (unsigned int i = 0; i < obj_set.size(); i++) {
        d = obj_set.at(i)->send(reflected_ray);
        if (d <= dist) {
            // The light is blocked by some object
            return rt::color::BLACK;
        }
    }

    //printf("source.cpp, apply_obj: index = %u\n", h.get_obj_index());
    const rt::color hit_color = obj_set.at(h.get_obj_index())->get_color();
    double cos_hit = (h.get_normal().unit() | (h.get_point() - position).unit());
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
