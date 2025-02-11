#include "scene/objects/sphere.hpp"
#include "light/vector.hpp"
#include "scene/material/material.hpp"
#include <math.h>

#include <optional>


/* Constructors */

sphere::sphere() {
    radius = 0;
}

sphere::sphere(const rt::vector& center, const real radius, const size_t material_index)

    : object(center, material_index), radius(radius) {}




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
           If t1 < 0 and t2 >= 0, this means the ray originates from inside the sphere,
           and t2 is returned.
           Otherwise, t1 < 0 and t2 < 0 means the sphere is behind the ray and is not hit. */
        
        const real sqrtdelta = sqrt(delta);
        const real t1 = dv - sqrtdelta;
        if (t1 >= 0.0f) {
            return t1;
        }
        else {
            const real t2 = dv + sqrtdelta;
            if (t2 >= 0.0f) {
                return t2;
            }
        }
    }
    
    return std::nullopt;
}

/* Returns the hit corresponding with the given intersection value t */
hit sphere::compute_intersection(ray& r, const real t) const {

    // Intersection point
    const rt::vector& u = r.get_origin();
    const rt::vector p = u + t * r.get_direction();
    
    const rt::vector n = (p - position) / radius;
    
    const object* pt_obj = this;
    ray* pt_ray = &r;
    return hit(pt_ray, p, n, pt_obj);
}


/* Minimum and maximum coordinates */
min_max_coord sphere::get_min_max_coord() const {
    
    const real min_x = position.x - radius;
    const real max_x = position.x + radius;

    const real min_y = position.y - radius;
    const real max_y = position.y + radius;

    const real min_z = position.z - radius;
    const real max_z = position.z + radius;

    return min_max_coord(min_x, max_x, min_y, max_y, min_z, max_z);
}