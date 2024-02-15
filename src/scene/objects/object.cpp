#include "headers/object.hpp"
#include "../material/headers/material.hpp"
#include <iostream>

/* Static members */

unsigned int object::counter = 0;
std::vector<const object*> object::set;

/* Constructors */

object::object()
    : position(rt::vector()), index(counter), mat(material()) {
    counter ++;
    set.push_back(this);
}

object::object(const rt::vector& position, const material& m)
    : position(position), index(counter), mat(m) {
    counter ++;
    set.push_back(this);
}



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

