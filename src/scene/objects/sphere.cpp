#include "scene/objects/sphere.hpp"

#include "tracing/direction.hpp"
#include "auxiliary/utils.hpp"
#include "math/geometry/trigonometry.hpp"

#include <iostream>
#include <cmath>

sphere::sphere(const rt::vector& center, const real radius, const unsigned int material_index,
    const unsigned int orientation_info_index)

    : object(center, material_index, orientation_info_index), radius(radius), radius_sq(radius * radius) {}

sphere::orientation::orientation(const mapping::index_type index,
    const rt::vector& forward_dir, const rt::vector& right_dir)

    : mapping_info(index) {

    const rt::vector forward = forward_dir.unit();
    const rt::vector right   = right_dir.unit();
    matrix = {
        .r1 = right,
        .r2 = right ^ forward,
        .r3 = forward
    };
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
    return hit(p, n, this, hit::compute_ray_orientation(r.direction, n), object_type::Sphere);
}


/* Minimum and maximum coordinates */
min_max_coord sphere::get_min_max_coord() const {

    const rt::vector r(radius, radius, radius);
    return build_min_max_coord(position - r, position + r);
}

uvcoord sphere::compute_uv(const rt::vector& p, const mapping_info* orientation_info) const {

    const orientation& o = *static_cast<const orientation*>(orientation_info);

    const rt::vector v = (p - position) / radius; // equal to normal: can it be optimized?
    const rt::vector oriented = o.matrix * v;
    const auto& [ theta, phi ] = trig::get_angles(oriented);
    // ST-coordinates:
    //  s = longitude, t = latitude
    //  On spheres, identical to UV-coordinates.
    return {
        theta * (1 / (2 * PI)),
        phi   * (1 / PI)
    };
}

/* Normal map vector computation at render time */
rt::vector sphere::compute_normal_from_map(const rt::vector& tangent_space_normal, const rt::vector& local_normal,
    const mapping_info* orientation_info) const {

    const orientation& o = *static_cast<const orientation*>(orientation_info);

    // Computation of tangent space
    const auto& [ _, up_dir, _ ] = o.matrix;
    const rt::vector t = (up_dir ^ local_normal).unit();
    const rt::vector b = t ^ local_normal;

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

void sphere::print() const {
    printf("Sphere: ");
    printf("center: "); position.print();
    printf(", radius: %lf\n", radius);
}