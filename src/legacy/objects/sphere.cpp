#include "legacy/objects/sphere.hpp"
#include <math.h>


/* Constructors */

sphere::sphere() {
    radius = 0;
}

sphere::sphere(const rt::vector& center, const real radius, const rt::color& col)

    : object(center, col), radius(radius) {}




/* Intersection determination */

/* Calculates and returns the intersection value t */
std::optional<real> sphere::measure_distance(const ray& r) const {
    /*
      v is the vector from the origin of the ray to the center of the sphere.
      dir is the direction of the ray (|dir| = 1).
      We have to solve the equation |v - t.dir|^2 = radius^2
      The system is equivalent to:
      t^2*|dir|^2 - 2(dir|v)t + |v|^2 - radius^2 = 0
      Delta = 4 * ((dir|v)^2 - 4 * |dir|^2 * (|v|^2 - radius^2))

    */

    const rt::vector v = position - r.get_origin();
    const rt::vector dir = r.get_direction(); // the direction is assumed to be a unit vector

    const real nv2 = v.normsq();
    const real dv = (dir | v);

    const real delta = dv * dv + radius * radius - nv2;
    // delta is actually the discriminant divided by 4

    if (delta >= 0.0f) {
        /* Two solutions: t1 = dv - sqrt(delta) and t2 = dv + sqrt(delta),
           If t1 >= 0, this means the ray originates from outside the sphere
           and the sphere is in front of the origin, and thus t1 is returned,
           If t1 < 0 and t2 >= 0, this means the ray originates from inside the sphere.
           Otherwise, t1 < 0 and t2 < 0 means the sphere is behind the ray and is not hit. */
    
        const real t1 = dv - sqrt(delta);
        if (t1 >= 0.0f) {
            return t1;
        }
    }
    return std::nullopt;
}

/* Returns the hit corresponding with the given intersection value t */
hit sphere::compute_intersection(ray& r, const real t) const {

    // Intersection point
    const rt::vector p = r.get_origin() + t * r.get_direction();
    
    const rt::vector n = (p - position) / radius;
    
    const object* pt_obj = this;
    
    return hit(p, n, pt_obj);
}
