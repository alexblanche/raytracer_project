#include "scene/objects/plane.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"
#include "auxiliary/utils.hpp"

#include <cmath>
#include <stdexcept>
#include <optional>

/* Main constructor */
/* A plane (P) of equation (P): ax + by + cz + d = 0
 defined by 4 reals a,b,c,d */
/* The normal vector (a, b, c) is a unit vector */
plane::plane(const real pa, const real pb, const real pc, const real pd,
    const unsigned int material_index)
    
    : object(rt::vector(0, 0, 0), material_index) {

    /* Normalization of the normal vector */
    const rt::vector normal_vector(pa, pb, pc);
    normal = normal_vector.unit();
    const auto [ a, b, c ] = normal;
    d = pd / normal_vector.norm();
    
    position =
          (is_not_zero(pa)) ? rt::vector(-d/a, 0, 0)
        : (is_not_zero(pb)) ? rt::vector(0, -d/b, 0)
        : (is_not_zero(pc)) ? rt::vector(0, 0, -d/c)
        :                     rt::vector(0, 0, 0);
}

/* Constructor of a plane of normal vector (a,b,c) and touching the point v */
plane::plane(const real pa, const real pb, const real pc, const rt::vector& position,
    const unsigned int material_index)

    : object(position, material_index),
      normal(rt::vector(pa, pb, pc).unit()),
      d(-(normal | position)) {} // = -aX-bY-cZ if position = (X,Y,Z)

// Constructor for textured planes
plane::plane(const real pa, const real pb, const real pc, const rt::vector& position,
    const unsigned int material_index,
    const unsigned int texture_info_index, const rt::vector& right, const real scale)

    : object(position, material_index, texture_info_index),
      normal(rt::vector(pa, pb, pc).unit()),
      d(-(normal | position)) {

    const rt::vector right_dir = right.unit();
    orientation = {
        .right_dir         = right_dir,
        .down_dir          = right_dir ^ normal,
        .inv_texture_scale = 1.0_r / scale
    };
}

/* Intersection determination */

real plane::measure_distance(const ray& r) const {

    /* Origin of the ray:    u   = (X, Y, Z)
       Direction of the ray: dir = (x, y, z)
       Normal of the plane:  n   = (a, b, c)
       The normal and the direction are supposed to be unit vectors.
       
       We search t so that u + t.dir belongs to the plane,
       i.e. a(X + tx) + b(Y + ty) + c(Z + tz) + d = 0,
       => t = -(aX + bY + cZ + d) / (ax + by + cz)
    */

    const real pdt  = (normal | r.direction);  // ax + by + cz
    const real upln = (normal | r.origin) + d; // aX + bY + cZ + d
    
    // If -upln/pdt > 0, it is our solution t, otherwise the plane is either parallel (pdt == 0) or "behind" the plane (-upln/pdt < 0)
    
    return (is_negative_not_zero(pdt * upln)) ? (- upln / pdt) : infinity;
}

hit plane::compute_intersection(const ray& r, const real t) const {

    // Intersection point
    //const rt::vector p = r.origin + t * r.direction;
    const rt::vector p = fma(r.direction, t, r.origin);

    // The normal vector (a, b, c) is assumed to be a unit vector

    const object* pt_obj = this;
    const ray* pt_ray = &r;
    return hit(pt_ray, p, normal, pt_obj);
}

/* Returns the barycentric info (tiles according to texture_scale) */
barycentric_info plane::get_barycentric(const rt::vector& p) const {

    const real right_component = (p | orientation.right_dir) * orientation.inv_texture_scale;
    real x_value = fmod(right_component, 1.0_r);
    if (is_negative(x_value)) x_value += 1.0_r;

    const real down_component = (p | orientation.down_dir) * orientation.inv_texture_scale;
    real y_value = fmod(down_component, 1.0_r);
    if (is_negative(y_value)) y_value += 1.0_r;

    return barycentric_info(x_value, y_value, object_type::Plane);
}

/* Normal map vector computation at render time */
rt::vector plane::compute_normal_from_map(const rt::vector& tangent_space_normal, const rt::vector& local_normal, const texture_info& /* info */) const {

    const auto& [ right, down, _ ] = orientation;
    
    return matprod(right, down, local_normal, tangent_space_normal);
}

/* Minimum and maximum coordinates */
min_max_coord plane::get_min_max_coord() const {

    throw std::runtime_error("Min/max coordinates undefined for planes");
}


rt::vector plane::sample(randomgen&) const {
    throw std::runtime_error("Sampling is unavailable for planes");
}


rt::vector plane::sample_visible(randomgen&, const rt::vector&) const {
    throw std::runtime_error("Sampling is unavailable for planes");
}