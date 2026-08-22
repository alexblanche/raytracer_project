#include "scene/objects/plane.hpp"

#include "auxiliary/utils.hpp"

#include <cmath>
#include <stdexcept>

/* Constructor of a plane of normal vector (a,b,c) and touching the point position */
plane::plane(const real pa, const real pb, const real pc, const rt::vector& position,
    const unsigned int material_index, const unsigned int orientation_info_index)

    : object(position, material_index, orientation_info_index),
      normal(rt::vector(pa, pb, pc).unit()),
      d(-(normal | position)) {} // = -aX-bY-cZ if position = (X,Y,Z)

/* A plane (P) of equation (P): ax + by + cz + d = 0
   defined by 4 reals a,b,c,d */
/* The normal vector (a, b, c) is a unit vector */
plane::plane(const real pa, const real pb, const real pc, const real pd,
    const unsigned int material_index, const unsigned int orientation_info_index)
    
    : plane(pa, pb, pc,
        /* d = */ (
              (is_not_zero(pa)) ? rt::vector(-pd / pa, 0, 0)
            : (is_not_zero(pb)) ? rt::vector(0, -pd / pb, 0)
            : (is_not_zero(pc)) ? rt::vector(0, 0, -pd / pc)
            :                     rt::ZERO
        ),
        material_index, orientation_info_index) {}

plane::plane(const rt::vector& normal, const rt::vector& position,
    const unsigned int material_index, const unsigned int orientation_info_index)

    : object(position, material_index, orientation_info_index),
      normal(normal.unit()), d(-(normal | position)) {}

plane::orientation::orientation(const mapping::index_type index,
    const rt::vector& normal, const rt::vector& right, real scale)

    : mapping_info(index),
      right_dir(right.unit()),
      down_dir(right_dir ^ normal),
      inv_texture_scale(1.0_r / scale) {}

/* Intersection determination */

real plane::measure_distance(const ray& r) const {
    // see Math-details.md

    const real pdt  = (normal | r.direction);
    const real upln = (normal | r.origin) + d;
    
    return (std::signbit(pdt) != std::signbit(upln)) ?
          - upln / pdt
        : infinity;
}

hit plane::compute_intersection(const ray& r, const real t) const {

    // Intersection point
    const rt::vector p = r.extend(t);
    return hit(p, normal, this, hit::compute_ray_orientation(r.direction, normal), object_type::Plane);
}

uvcoord plane::compute_uv(const rt::vector& p, const mapping_info* orientation_info) const {

    const orientation& o = *static_cast<const orientation*>(orientation_info);
    
    const real right_component = (p | o.right_dir) * o.inv_texture_scale;
    real x_value = fmod(right_component, 1.0_r);
    if (is_negative(x_value)) x_value += 1.0_r;

    const real down_component = (p | o.down_dir) * o.inv_texture_scale;
    real y_value = fmod(down_component, 1.0_r);
    if (is_negative(y_value)) y_value += 1.0_r;

    // ST-coordinates: on planes, identical to UV-coordinates
    return { x_value, y_value };
}

/* Normal map vector computation at render time */
rt::vector plane::compute_normal_from_map(const rt::vector& tangent_space_normal, const rt::vector& local_normal,
    const mapping_info* orientation_info) const {

    const orientation& o = *static_cast<const orientation*>(orientation_info);
    
    return matprod(o.right_dir, o.down_dir, local_normal, tangent_space_normal);
}

/* Minimum and maximum coordinates */
min_max_coord plane::get_min_max_coord() const {

    throw std::runtime_error("Min/max coordinates undefined for planes");
}


rt::vector plane::sample(const randomgen&) const {
    static_assert(TODO_PLANE_SAMPLING);
    throw std::runtime_error("Sampling is unavailable for planes");
}


rt::vector plane::sample_visible(const randomgen&, const rt::vector&) const {
    static_assert(TODO_PLANE_SAMPLING);
    throw std::runtime_error("Sampling is unavailable for planes");
}

void plane::print() const {
    printf("Plane: ");
    printf("normal: ");
    normal.print();
    printf(", position: ");
    position.print();
    printf("\n");
}