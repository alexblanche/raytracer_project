#include "scene/objects/sphere.hpp"
#include "light/vector.hpp"
#include "scene/material/material.hpp"
#include "tracing/directions.hpp"
#include "auxiliary/utils.hpp"

#include <cmath>
#include <optional>

sphere::sphere(const rt::vector& center, const real radius, const unsigned int material_index)

    : object(center, material_index), radius(radius) {}

// Constructor for textured spheres
sphere::sphere(const rt::vector& center, const real radius, const unsigned int material_index,
    const unsigned int texture_info_index, const rt::vector& forward, const rt::vector& right)

    : object(center, material_index, texture_info_index), radius(radius) {
        // forward_dir(forward.unit()), right_dir(right.unit())
        // up_dir = right_dir.value() ^ forward_dir.value();

        orientation.forward_dir = forward.unit();
        orientation.right_dir   = right.unit();
        orientation.up_dir = orientation.right_dir ^ orientation.forward_dir;
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

    const rt::vector v = position - r.origin;
    const real nv2 = v.normsq();
    const real dv = (r.direction | v); // the direction is assumed to be a unit vector

    const real delta = dv * dv + radius * radius - nv2;
    // delta is actually the discriminant divided by 4

    if (is_negative(delta))
        return std::nullopt;

    
    /* Two solutions: t1 = dv - sqrt(delta) and t2 = dv + sqrt(delta),
        If t1 >= 0, this means the ray originates from outside the sphere
        and the sphere is in front of the origin, and thus t1 is returned,
        If t1 < 0 and t2 >= 0, this means the ray originates from inside the sphere,
        and t2 is returned.
        Otherwise, t1 < 0 and t2 < 0 means the sphere is behind the ray and is not hit. */
    
    const real sqrtdelta = sqrt(delta);
    const real t1 = dv - sqrtdelta;
    
    return (is_positive(t1))          ? std::optional(t1)
        :  (t1 >= -2.0_r * sqrtdelta) ? std::optional(dv + sqrtdelta)
        :                               std::nullopt;
}

/* Returns the hit corresponding with the given intersection value t */
hit sphere::compute_intersection(const ray& r, const real t) const {

    // Intersection point
    const rt::vector p = fma(r.direction, t, r.origin);
    // Normal at intersection point
    const rt::vector n = (p - position) / radius;
    return hit(&r, p, n, this);
}


/* Minimum and maximum coordinates */
min_max_coord sphere::get_min_max_coord() const {
    
    return {
        .min_x = position.x - radius,
        .max_x = position.x + radius,

        .min_y = position.y - radius,
        .max_y = position.y + radius,

        .min_z = position.z - radius,
        .max_z = position.z + radius
    };
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
    return matprod(
        t,            tangent_space_normal.x,
        b,            tangent_space_normal.y,
        local_normal, tangent_space_normal.z
    );
}

/* Sampling */

/* Uniformly samples a point on the sphere */
rt::vector sphere::sample(randomgen& rg) const {
    //return position + (radius * random_direction(rg, rt::vector(0, 1, 0), PI));
    return //fma(random_direction<PI>(rg, rt::vector(0, 1, 0)), radius, position);
        fma(random_direction(rg, rt::vector(0, 1, 0), PI), radius, position);
}

/* Uniformly samples a point on the sphere that is visible from pt */
rt::vector sphere::sample_visible(randomgen& rg, const rt::vector& pt) const {
    /*
    const rt::vector v = sample(rg);
    if (((v - position) | (pt - position)) >= 0.0_r)
        return v;
    else
        return 2.0_r * position - v;
    */
    // Optimized
    return random_direction(rg, (pt - position).unit(), PI / 2.0_r);
}