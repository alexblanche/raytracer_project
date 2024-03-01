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



/* Default versions: these four functions are overridden by derived classes */

/* Intersection determination */

double object::measure_distance(const ray& r) const {
    return (r.get_origin() - position).norm();
}

hit object::compute_intersection(const ray& r, const double t) const {
    rt::vector p = r.get_origin() + t * r.get_direction().unit();
    rt::vector n = (-1)*(r.get_direction().unit());
    return hit(r, p, n, index);
}

/* Writes the barycentric coordinates in variables l1, l2
   The boolean return value is used for determining the three points considered in quads */
bool object::get_barycentric(const rt::vector& p, double& l1, double& l2) const {
    return true;
}

/* Writes the minimum and maximum coordinates of the object on the three axes */
void object::min_max_coord(double& min_x, double& max_x,
    double& min_y, double& max_y, double& min_z, double& max_z) const {

    return;
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