#include "scene/objects/sphere.hpp"
#include "light/vector.hpp"
#include "scene/material/material.hpp"
#include <cmath>

#include <optional>

#define PI 3.14159265359f
#define INVPI 0.31830988618f


/* Constructors */

sphere::sphere() {
    radius = 0;
}

sphere::sphere(const rt::vector& center, const real radius, const size_t material_index)

    : object(center, material_index), radius(radius), forward_dir(std::nullopt), right_dir(std::nullopt) {}

// Constructor for textured spheres
sphere::sphere(const rt::vector& center, const real radius, const size_t material_index,
    const std::optional<texture_info>& info, const rt::vector& forward, const rt::vector& right)

    : object(center, material_index, info), radius(radius),
        forward_dir(forward.unit()), right_dir(right.unit()) {

        up_dir = right_dir.value() ^ forward_dir.value();
    }




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

/* Returns the barycentric info for the object (l1 = longitude, l2 = latitude) (both between 0 and 1) */
barycentric_info sphere::get_barycentric(const rt::vector& p) const {
    const rt::vector v = (p - position).unit();
    const real forward_component = (v | forward_dir.value());
    const real right_component   = (v | right_dir.value());
    const real up_component      = (v | up_dir.value());

    const real theta = asin(up_component);
    const real x = acos(forward_component / cos(theta));
    const real phi = (right_component < 0.0f) ? -x + 2.0f * PI : x;

    return barycentric_info(phi * INVPI * 0.5f, theta * INVPI + 0.5f);
}

/* Normal map vector computation at render time */
rt::vector sphere::compute_normal_from_map(const rt::vector tangent_space_normal, const rt::vector local_normal) const {

    // Computation of tangent space
    const rt::vector t = up_dir.value() ^ local_normal;
    const rt::vector b = t ^ local_normal;

    return tangent_space_normal.x * t + tangent_space_normal.y * b + tangent_space_normal.z * local_normal;
}