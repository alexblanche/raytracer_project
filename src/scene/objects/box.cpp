#include "scene/objects/box.hpp"

#include "light/hit.hpp"
#include "auxiliary/utils.hpp"

#include <stdexcept>


/* Intersection determination */

real box::measure_distance(const ray& r) const {

    const auto& [ u, dir, _ ] = r;
    const auto& [ l1, l2, l3 ] = dims;

    const rt::vector u_b   = axes * (u - position);
    const rt::vector dir_b = axes * dir;

    const auto& [   u_x,   u_y,   u_z ] = u_b;
    const auto& [ dir_x, dir_y, dir_z ] = dir_b;

    // Factor that depends on whether u is outside or inside the box
    const real a = (std::abs(u_x) <= l1 && std::abs(u_y) <= l2 && std::abs(u_z) <= l3) ?
          /* inside */   1.0_r
        : /* outside */ -1.0_r;
    
    if (is_not_zero(dir_x)) {
        const real t_x = (-u_x + a * copysign(l1, dir_x)) / dir_x;
        // Check that t_x gives a point inside the face
        if (std::abs(fma(dir_y, t_x, u_y)) <= l2 && std::abs(fma(dir_z, t_x, u_z)) <= l3)
            return is_positive(t_x) ? t_x : infinity;
    }

    if (is_not_zero(dir_y)) {
        const real t_y = (-u_y + a * copysign(l2, dir_y)) / dir_y;
        if (std::abs(fma(dir_x, t_y, u_x)) <= l1 && std::abs(fma(dir_z, t_y, u_z)) <= l3)
            return is_positive(t_y) ? t_y : infinity;
    }
    
    if (is_not_zero(dir_z)) {
        const real t_z = (-u_z + a * copysign(l3, dir_z)) / dir_z;
        return (is_positive(t_z)
            && std::abs(fma(dir_x, t_z, u_x)) <= l1 && std::abs(fma(dir_y, t_z, u_y)) <= l2) ?
            t_z : infinity;
    }

    return infinity;
}
        
hit box::compute_intersection(const ray& r, const real t) const {
    // Intersection point
    const rt::vector p = r.extend(t);

    // Re-computing the face of intersection
    // (not great, but the alternative is to return a hit object (with distance t) for every intersection check)

    // Shifting the position a little bit, to avoid the ray hitting the object itself again
    const rt::vector v = p - position;
    const object* pt_obj = this;

    constexpr real EPSILON = 0.0000001_r;

    rt::vector n;
    const auto& [ n1, n2, n3 ] = axes;
    const auto& [ l1, l2, l3 ] = dims;

    const real u_x = (v | n1);
    if (std::abs(u_x - l1) < EPSILON)
        n = n1;
    else if (std::abs(u_x + l1) < EPSILON)
        n = (-1.0_r) * n1;
    else {

        const real u_y = (v | n2);
        if (std::abs(u_y - l2) < EPSILON)
            n = n2;
        else if (std::abs(u_y + l2) < EPSILON)
            n = (-1.0_r) * n2;
        else {

            const real u_z = (v | n3);
            n = (std::abs(u_z - l3) < EPSILON) ?
                n3 : (-1.0_r) * n3;
        }
    }

    return hit(p, n, pt_obj, hit::compute_ray_orientation(r.direction, n), object_type::Box);
}

/* Minimum and maximum coordinates */
min_max_coord box::get_min_max_coord() const {

    const auto& [ n1, n2, n3 ] = axes;

    const rt::vector absn1 = rt::abs(n1);
    const rt::vector absn2 = rt::abs(n2);
    const rt::vector absn3 = rt::abs(n3);

    const rt::vector m = matprod(absn1, absn2, absn3, dims);
    
    return build_min_max_coord(position - m, position + m);
}


/* Specific to AABB: returns true if the ray r hits the box
   The box is assumed to be standard (axes are n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1)) */
bool box::is_hit_by(const ray& r) const {
    
    const auto& [ u, dir, inv_dir ] = r;
    const auto& [ l1, l2, l3 ] = dims;

    // See measure_distance

    const auto& [ u_x, u_y, u_z ] = u - position;

    if (std::abs(u_x) <= l1 && std::abs(u_y) <= l2 && std::abs(u_z) <= l3) {
        // u is inside the box
        return true;
    }

    if (is_not_zero(dir.x)) {
        const real t_x = (-u_x - copysign(l1, dir.x)) * inv_dir.x;
        // Check that t_x gives a point inside the face
        if (std::abs(fma(dir.y, t_x, u_y)) <= l2 && std::abs(fma(dir.z, t_x, u_z)) <= l3)
            return is_positive(t_x);
    }

    if (is_not_zero(dir.y)) {
        const real t_y = (-u_y - copysign(l2, dir.y)) * inv_dir.y;
        if (std::abs(fma(dir.x, t_y, u_x)) <= l1 && std::abs(fma(dir.z, t_y, u_z)) <= l3)
            return is_positive(t_y);
    }
    
    const real t_z = (-u_z - copysign(l3, dir.z)) * inv_dir.z;
    return (is_positive(t_z)
        && std::abs(fma(dir.x, t_z, u_x)) <= l1 && std::abs(fma(dir.y, t_z, u_y)) <= l2);
}

/* Specific to AABB: returns true if the ray r hits the box
   The box is assumed to be an AABB */
real box::is_hit_with_distance(const ray& r) const {
    
    const auto& [ u, dir, inv_dir ] = r;
    const auto& [ l1, l2, l3 ] = dims;

    // See measure_distance

    const auto& [ u_x, u_y, u_z ] = u - position;
    
    if (std::abs(u_x) <= l1 && std::abs(u_y) <= l2 && std::abs(u_z) <= l3) {
        // u is inside the box
        return 0.0_r;
    }

    const real t_x = (-u_x - copysign(l1, dir.x)) * inv_dir.x;
    if (std::abs(fma(dir.y, t_x, u_y)) <= l2 && std::abs(fma(dir.z, t_x, u_z)) <= l3)
        return t_x > 0 ? t_x : infinity;

    const real t_y = (-u_y - copysign(l2, dir.y)) * inv_dir.y;
    if (std::abs(fma(dir.x, t_y, u_x)) <= l1 && std::abs(fma(dir.z, t_y, u_z)) <= l3)
        return t_y > 0 ? t_y : infinity;

    const real t_z = (-u_z - copysign(l3, dir.z)) * inv_dir.z;
    return (t_z > 0
        && std::abs(fma(dir.x, t_z, u_x)) <= l1 && std::abs(fma(dir.y, t_z, u_y)) <= l2) ?
        t_z : infinity;
}

// Texturing is unavailable for boxes
uvcoord box::compute_uv(const rt::vector&, const mapping_info*) const {
    static_assert(TODO_BOX_TEXTURING);
    throw std::runtime_error("Texturing is unavailable for boxes");
}

rt::vector box::compute_normal_from_map(
            const rt::vector&,
            const rt::vector&,
            const mapping_info*
        ) const {

    static_assert(TODO_BOX_TEXTURING);
    throw std::runtime_error("Texturing is unavailable for boxes");
}

rt::vector box::sample(const randomgen&) const {
    static_assert(TODO_BOX_SAMPLING);
    throw std::runtime_error("Sampling is unavailable for boxes");
}

rt::vector box::sample_visible(const randomgen&, const rt::vector&) const {
    static_assert(TODO_BOX_SAMPLING);
    throw std::runtime_error("Sampling is unavailable for boxes");
}

void box::print() const {
    printf("Box: ");
    printf("center: ");
    position.print();
    printf(", dimensions: ");
    const rt::vector total_dims = 2 * dims;
    total_dims.print();
    printf("\n");
}