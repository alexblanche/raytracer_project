#include "headers/object.hpp"

/* Constructors */

object::object() {
    position = rt::vector();
    color = rt::color::WHITE;
}

object::object(const rt::vector& position, const rt::color& color)
    : position(position), color(color) {}



/* Accessors */

rt::vector object::get_position() const {
    return position;
}

rt::color object::get_color() const {
    return color;
}

/* Intersection determination */

/* Default versions: these two functions are overridden by derived classes */

double object::send(const ray& r) const {
    return (r.get_origin() - position).norm();
}

hit object::intersect(const ray& r, double t) const {
    rt::vector p = r.get_origin() + t * r.get_direction().unit();
    rt::vector n = (-1)*(r.get_direction().unit());
    return hit(r, p, n, color);
}