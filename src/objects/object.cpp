#include "headers/object.hpp"
#include <iostream>

/* Constructors */

object::object() {
    position = rt::vector();
    color = rt::color::WHITE;
    index = 0;
}

object::object(const rt::vector& position, const rt::color& color, const unsigned int i)
    : position(position), color(color), index(i) {}



/* Accessors */

rt::vector object::get_position() const {
    return position;
}

rt::color object::get_color() const {
    return color;
}

unsigned int object::get_index() const {
    return index;
}

/* Intersection determination */

/* Default versions: these two functions are overridden by derived classes */

double object::send(const ray& r) const {
    return (r.get_origin() - position).norm();
}

hit object::intersect(const ray& r, const double t) const {
    rt::vector p = r.get_origin() + t * r.get_direction().unit();
    rt::vector n = (-1)*(r.get_direction().unit());
    //printf("object.cpp, intersect: index = %u \n", index);
    return hit(r, p, n, index);
}
