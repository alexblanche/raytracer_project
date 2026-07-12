#include "scene/objects/sphere.hpp"
#include "light/vector.hpp"
#include "scene/material/material.hpp"
#include "tracing/direction.hpp"
#include "auxiliary/utils.hpp"

#include <cmath>
#include <optional>

sphere::sphere(const rt::vector& center, const real radius, const unsigned int material_index)

    : object(center, material_index), radius(radius), radius_sq(radius * radius) {}

// Constructor for textured spheres
sphere::sphere(const rt::vector& center, const real radius, const unsigned int material_index,
    const unsigned int texture_info_index, const rt::vector& forward, const rt::vector& right)

    : sphere(center, radius, material_index) {
    
        texture_information_index = texture_info_index;
        orientation.forward_dir = forward.unit();
        orientation.right_dir   = right.unit();
        orientation.up_dir = orientation.right_dir ^ orientation.forward_dir;
    }




/* Intersection determination */

/* Calculates and returns the intersection value t */
real sphere::measure_distance(const ray& r) const {
    /*
      v is the vector from the origin of the ray to the center of the sphere.
      dir is the direction of the ray (|dir| = 1).
      We have to solve the equation |v - t.dir|^2 = radius^2
      The system is equivalent to:
      t^2*|dir|^2 - 2(dir|v)t + |v|^2 - radius^2 = 0
      Delta = 4 * ((dir|v)^2 - 4 * |dir|^2 * (|v|^2 - radius^2))
    */

    const rt::vector v = position - r.origin;
    const real nv2 = v.normsq();
    const real dv = (r.direction | v); // the direction is assumed to be a unit vector

    const real delta = dv * dv + radius_sq - nv2;
    // delta is actually the discriminant divided by 4

    if (is_negative(delta))
        return infinity;

    /* Two solutions: t1 = dv - sqrt(delta) and t2 = dv + sqrt(delta),
        If t1 >= 0, this means the ray originates from outside the sphere
        and the sphere is in front of the origin, and thus t1 is returned,
        If t1 < 0 and t2 >= 0, this means the ray originates from inside the sphere,
        and t2 is returned.
        Otherwise, t1 < 0 and t2 < 0 means the sphere is behind the ray and is not hit. */
    
    const real sqrtdelta = sqrt(delta);
    const real t1 = dv - sqrtdelta;
    real t2;
    
    return (is_positive(t1))                ? t1
        :  ((t2 = dv + sqrtdelta) >= 0.0_r) ? t2
        :                                     infinity;
}

/* Returns the hit corresponding with the given intersection value t */
hit sphere::compute_intersection(const ray& r, const real t) const {

    // Intersection point
    const rt::vector p = r.extend(t);
    // Normal at intersection point
    const rt::vector n = (p - position) / radius;
    return hit(r.direction, p, n, this);
}


/* Minimum and maximum coordinates */
min_max_coord sphere::get_min_max_coord() const {

    const rt::vector r(radius, radius, radius);
    return build_min_max_coord(position - r, position + r);
}

/* Returns the barycentric info for the object (l1 = longitude, l2 = latitude) (both between 0 and 1) */
barycentric_info sphere::get_barycentric(const rt::vector& p) const {
    const rt::vector v = (p - position).unit();
    const real forward_component = (v | orientation.forward_dir);
    const real right_component   = (v | orientation.right_dir  );
    const real up_component      = (v | orientation.up_dir     );

    const real theta = asin(up_component);
    const real x = acos(forward_component / cos(theta));
    const real phi = is_negative_not_zero(right_component) ? (-x + 2.0_r * PI) : x;

    return barycentric_info(
        phi   * ((1.0_r / PI) * 0.5_r),
        theta * (1.0_r / PI) + 0.5_r,
        object_type::Sphere
    );
}

/* Normal map vector computation at render time */
rt::vector sphere::compute_normal_from_map(const rt::vector& tangent_space_normal, const rt::vector& local_normal, const texture_info& /* info */) const {

    // Computation of tangent space
    const rt::vector t = (orientation.up_dir ^ local_normal).unit();
    const rt::vector b = t ^ local_normal;

    // return tangent_space_normal.x * t + tangent_space_normal.y * b + tangent_space_normal.z * local_normal;
    return matprod(t, b, local_normal, tangent_space_normal);
}

/* Sampling */

using enum direction::angle;

/* Uniformly samples a point on the sphere */
rt::vector sphere::sample(const randomgen& rg) const {
    const rt::vector central_dir = direction::random<Pi>(rg);
    return fma(central_dir, radius, position);
}

/* Uniformly samples a point on the sphere that is visible from pt */
rt::vector sphere::sample_visible(const randomgen& rg, const rt::vector& pt) const {
    return direction::random<Pi_over_2>(rg, (pt - position).unit());
}