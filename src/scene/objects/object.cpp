#include "headers/object.hpp"
#include "../material/headers/material.hpp"
#include <iostream>

/* Constructors */

object::object() {
    position = rt::vector();
    index = 0;
    mat = material();
}

object::object(const rt::vector& position, const unsigned int i, const material& m)

    : position(position), index(i), mat(m) {}



/* Accessors */

rt::vector object::get_position() const {
    return position;
}

unsigned int object::get_index() const {
    return index;
}

material object::get_material() const {
    return mat;
}

/* Intersection determination */

/* Default versions: these two functions are overridden by derived classes */

double object::measure_distance(const ray& r) const {
    return (r.get_origin() - position).norm();
}

hit object::compute_intersection(const ray& r, const double t) const {
    rt::vector p = r.get_origin() + t * r.get_direction().unit();
    rt::vector n = (-1)*(r.get_direction().unit());
    return hit(r, p, n, index);
}

