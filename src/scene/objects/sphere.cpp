#include "headers/sphere.hpp"
#include "../../light/headers/vector.hpp"
#include "../material/headers/material.hpp"
#include <math.h>

#include<limits>
numeric_limits<double> realsph;
const double infinity = realsph.infinity();


/* Constructors */

sphere::sphere() {
    radius = 0;
}

sphere::sphere(const rt::vector& center, const double radius, const material& material)
    : object(center, material), radius(radius) {}



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
      dir is the direction of the ray (||dir|| = 1).
      We have to solve the equation ||v - t.dir||^2 = radius^2
      The system is equivalent to:
      t^2*||dir||^2 - 2(dir|v)t + ||v||^2 - radius^2 = 0
      Delta = 4 * ((dir|v)^2 - 4 * ||dir||^2 * (||v||^2 - radius^2))

    */
    rt::vector v = get_center() - r.get_origin();
    rt::vector dir = r.get_direction(); // the direction is assumed to be a unit vector

    double nv2 = v.normsq();
    double dv = (dir | v);

    const double a = dv * dv + radius * radius - nv2;
    // Delta = 4*a

    if (a > 0) {
        /* Two solutions: t1 = dv - sqrt(a) and t2 = dv + sqrt(a),
           If t1 >= 0, this means the ray originates from outside the sphere
           and the sphere is in front of the origin, and thus t1 is returned,
           If t1 < 0 and t2 >= 0, this means the ray originates from inside the sphere,
           and t2 is returned.
           Otherwise, t1 < 0 and t2 < 0 means the sphere is behind the ray and is not hit. */
        const double t1 = dv - sqrt(a);
        if (t1 >= 0) {
            return t1;
        }
        else {
            const double t2 = dv + sqrt(a);
            if (t2 >= 0) {
                return t2;
            }
            else {
                return infinity;
            }
        }
    }
    else {
        return infinity;
    }
}

/* Returns the hit corresponding with the given intersection value t */
hit sphere::compute_intersection(const ray& r, const double t) const {

    // Intersection point
    const rt::vector u = r.get_origin();
    const rt::vector p = u + t * r.get_direction();
    
    // Normal vector
    const double radius = get_radius();
    const rt::vector c = get_center();
    if ((u - c).normsq() > radius * radius) {
        // The ray originates from outside the sphere
        const rt::vector n = (p - c) / radius;
        return hit(r, p, n, get_index());
    }
    else {
        // The rays originates from inside the sphere
        const rt::vector n = (c - p) / radius;
        return hit(r, p, n, get_index());
    }
}

