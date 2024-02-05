#include "headers/sphere.hpp"
#include "../../light/headers/vector.hpp"
#include "../material/headers/material.hpp"
#include <math.h>

#include<limits>
numeric_limits<double> realsph;
const double infinity = realsph.infinity();


/* Constructors */

sphere::sphere(const rt::vector& center, const double radius, const unsigned int index,
    const material& material)
    : object(center, index, material), radius(radius) {}

sphere::sphere() {
    radius = 0;
}

/* Accessors */

rt::vector sphere::get_center() const {
    return position;
}

double sphere::get_radius() const {
    return radius;
}


/* Intersection determination */

/* Calculates and returns the intersection value t */
double sphere::measure_distance(const ray& r) const {
    /*
      v is the vector from the origin of the ray to the center of the sphere.
      u is the direction of the ray (||u|| = 1).
      We have to solve the equation ||v - t.u||^2 = radius^2
      The system is equivalent to:
      t^2*||u||^2 - 2(u|v)t + ||v||^2 - radius^2 = 0
      Delta = 4 * ((u|v)^2 - 4 * ||u||^2 * (||v||^2 - radius^2))

    */
    rt::vector v = get_center() - r.get_origin();
    rt::vector u = r.get_direction(); // the direction is assumed to be a unit vector

    double nv2 = v.normsq();
    double uv = (u | v);

    const double a = uv * uv + radius * radius - nv2;
    // Delta = 4*a

    if (a > 0) {
        const double t1 = uv - sqrt(a);
        //double t2 = uv + sqrt(A);

        if (t1 > 0) { // t2 > 0 because t2 > t1
            return t1; // = min(t1,t2)
        }
        /*
        else if (t2>0) {
            return t2;
        }
        */
        else {
            return infinity;
        }
    }
    else {
        return infinity;
    }
}

/* Returns the hit corresponding with the given intersection value t */
hit sphere::compute_intersection(const ray& r, const double t) const {

    // Intersection point
    rt::vector p = r.get_origin() + t * r.get_direction();
    
    // Normal vector
    rt::vector n = (p - get_center()).unit();

    return hit(r, p, n, get_index());
}

