#include "headers/object.hpp"
#include "../material/headers/material.hpp"
#include <iostream>

#include <limits>
std::numeric_limits<double> real;
const double infinity = real.infinity();

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



/* Computing the intersection of a ray with the scene */
hit object::find_closest_object(const ray& r) {
    
    double closest = infinity;
    unsigned int closest_index = -1;

    // Looking for the closest object
    for (unsigned int i = 0; i < object::counter; i++) {
        
        // We do not test the intersection with the object the rays is cast from
        const double d = object::set.at(i)->measure_distance(r);
                
        /* d is the distance between the origin of the ray and the
           intersection point with the object */

        if (d < closest && d > 0.000001) {
            closest = d;
            closest_index = i;
        }
    }

    if (closest_index == ((unsigned int) -1)) {
        return hit();
    }
    else {
        return object::set.at(closest_index)->compute_intersection(r, closest);
    }
}