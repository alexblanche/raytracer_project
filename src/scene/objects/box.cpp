#include "scene/objects/box.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "auxiliary/utils.hpp"

#include <stdexcept>


/* Intersection determination */

real box::measure_distance(const ray& r) const {

    /* For the face orthogonal to n1, we search for a t1 that satisfies:
       ((pos + a.l1.n1) - (u + t.dir) | n1) = 0 (if outside the box, a = -sign(dir|n1), if inside: a = sign(dir|n1))
       where u is the origin of the ray, and dir its direction,
       So t1 = ((pos + l1.n1 - u) | n1) / (dir | n1)
       We then check whether |((pos + l1.n1 - (u + t1.dir)) | n2)| <= l2 and |(((pos + l1.n1 - (u + t1.dir)) | n3)| <= l3,
       to make sure the intersection point lies on the face.

       Same with the other two directions, we obtain t2, t3.
       If (dir | n) = 0 for n among n1, n2, n3, then the ray does not intersect the plane and the associated t is infinity.

       Only one of t1, t2, t3 can be finite at the end, so we return one as soon as we find it.
    */

    const rt::vector& dir = r.direction;

    /**
     * const vector pmu = position - u;
     * t1 = ((c1 - u) | n1) / pdt1
     *      = ((position - (l1 * pdt1 / std::abs(pdt1)) * n1 - u) | n1) / pdt1
     *      = ((pmu | n1) - l1 * pdt1 / std::abs(pdt1)) / pdt1
     *      = ((pmu | n1) / pdt1 - l1 / std::abs(pdt1))
     *      = pmun1 / pdt1 - l1 / abspdt1
     * 
     * (p | n2)
     *      = ((c1 - u - (t1 * dir)) | n2)
     *      = ((position - (...)*n1 - u - t1 * dir) | n2)
     *      = (pmu | n2) - t1 * pdt2
     *      = pmnu2 - t1 * pdt2
     * **/

    const rt::vector pmu = position - r.origin;
    const real pmun1 = (pmu | n1);
    const real pmun2 = (pmu | n2);
    const real pmun3 = (pmu | n3);

    // Factor that depends on whether u is outside or inside the box
    const real a = (std::abs(pmun1) <= l1 && std::abs(pmun2) <= l2 && std::abs(pmun3) <= l3) ?
          /* inside */   1.0_r
        : /* outside */ -1.0_r;

    const real pdt1 = (dir | n1);
    const real pdt2 = (dir | n2);
    const real pdt3 = (dir | n3);

    if (is_not_zero(pdt1)) {
        // Determination of t1
        const real t1 = pmun1 / pdt1 + a * l1 / std::abs(pdt1);
        // Check that t1 gives a point inside the face
        if (std::abs(pmun2 - t1 * pdt2) <= l2 && std::abs(pmun3 - t1 * pdt3) <= l3)
            return (is_positive(t1)) ? t1 : infinity;
    }

    if (is_not_zero(pdt2)) {
        const real t2 = pmun2 / pdt2 + a * l2 / std::abs(pdt2);
        if (std::abs(pmun1 - t2 * pdt1) <= l1 && std::abs(pmun3 - t2 * pdt3) <= l3)
            return (is_positive(t2)) ? t2 : infinity;
    }

    if (is_not_zero(pdt3)) {
        const real t3 = pmun3 / pdt3 + a * l3 / std::abs(pdt3);
        return (is_positive(t3) && std::abs(pmun1 - t3 * pdt1) <= l1 && std::abs(pmun2 - t3 * pdt2) <= l2) ?
              t3 : infinity;
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

    const real pdt1 = (v | n1);
    if (std::abs(pdt1 - l1) < EPSILON)
        return hit(r.direction, p, n1, pt_obj);
    
    if (std::abs(pdt1 + l1) < EPSILON)
        return hit(r.direction, p, (-1.0_r) * n1, pt_obj);
    
    const real pdt2 = (v | n2);
    if (std::abs(pdt2 - l2) < EPSILON)
        return hit(r.direction, p, n2, pt_obj);
    
    if (std::abs(pdt2 + l2) < EPSILON)
        return hit(r.direction, p, (-1.0_r) * n2, pt_obj);
    
    const real pdt3 = (v | n3);
    if (std::abs(pdt3 - l3) < EPSILON)
        return hit(r.direction, p, n3, pt_obj);

    return hit(r.direction, p, (-1.0_r) * n3, pt_obj);
}

/* Minimum and maximum coordinates */
min_max_coord box::get_min_max_coord() const {

    const rt::vector absn1 = rt::abs(n1);
    const rt::vector absn2 = rt::abs(n2);
    const rt::vector absn3 = rt::abs(n3);

    const rt::vector m = matprod(absn1, absn2, absn3, get_l());
    
    return build_min_max_coord(position - m, position + m);
}


/* Specific to (standard) boxes: returns true if the ray r hits the box
   The box is assumed to be standard (axes are n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1)) */
bool box::is_hit_by(const ray& r) const {
    
    const auto& [ u, dir, inv_dir /* , abs_inv_dir */ ] = r;
    const rt::vector abs_inv_dir = rt::abs(inv_dir);

    // See measure_distance

    const rt::vector pmu = position - u;
    
    if (std::abs(pmu.x) <= l1 && std::abs(pmu.y) <= l2 && std::abs(pmu.z) <= l3) {
        // u is inside the box
        return true;
    }

    // Determination of t1
    const real t1 = pmu.x * inv_dir.x - l1 * abs_inv_dir.x;
    // Check that t1 gives a point inside the face
    if (std::abs(pmu.y - t1 * dir.y) <= l2 && std::abs(pmu.z - t1 * dir.z) <= l3)
        return is_positive(t1);

    const real t2 = pmu.y * inv_dir.y - l2 * abs_inv_dir.y;
    if (std::abs(pmu.x - t2 * dir.x) <= l1 && std::abs(pmu.z - t2 * dir.z) <= l3)
        return is_positive(t2);

    const real t3 = pmu.z * inv_dir.z - l3 * abs_inv_dir.z;
    return (is_positive(t3) && std::abs(pmu.x - t3 * dir.x) <= l1 && std::abs(pmu.y - t3 * dir.y) <= l2);
}

/* Specific to (standard) boxes: returns true if the ray r hits the box
   The box is assumed to be standard (axes are n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1)) */
real box::is_hit_with_distance(const ray& r) const {
    
    const auto& [ u, dir, inv_dir /* , abs_inv_dir */ ] = r;
    // const rt::vector abs_inv_dir = rt::abs(inv_dir);

    // See measure_distance

    const rt::vector pmu = position - u;
    
    if (std::abs(pmu.x) <= l1 && std::abs(pmu.y) <= l2 && std::abs(pmu.z) <= l3) {
        // u is inside the box
        return 0.0_r;
    }

    // const real t1 = pmu.x * inv_dir.x - l1 * std::abs(inv_dir.x);
    // if (std::abs(pmu.y - t1 * dir.y) <= l2 && std::abs(pmu.z - t1 * dir.z) <= l3)
    //     return t1 > 0 ? t1 : infinity;

    // const real t2 = pmu.y * inv_dir.y - l2 * std::abs(inv_dir.y);
    // if (std::abs(pmu.x - t2 * dir.x) <= l1 && std::abs(pmu.z - t2 * dir.z) <= l3)
    //     return t2 > 0 ? t2 : infinity;

    // const real t3 = pmu.z * inv_dir.z - l3 * std::abs(inv_dir.z);
    // return (t3 > 0 && std::abs(pmu.x - t3 * dir.x) <= l1 && std::abs(pmu.y - t3 * dir.y) <= l2) ?
    //     t3 : infinity;

    const real t1 = pmu.x * inv_dir.x - l1 * std::abs(inv_dir.x);
    if (std::abs(std::fma(dir.y, -t1, pmu.y)) <= l2 && std::abs(std::fma(dir.z, -t1, pmu.z)) <= l3)
        return t1 > 0 ? t1 : infinity;

    const real t2 = pmu.y * inv_dir.y - l2 * std::abs(inv_dir.y);
    if (std::abs(std::fma(dir.x, -t2, pmu.x)) <= l1 && std::abs(std::fma(dir.z, -t2, pmu.z)) <= l3)
        return t2 > 0 ? t2 : infinity;

    const real t3 = pmu.z * inv_dir.z - l3 * std::abs(inv_dir.z);
    return (t3 > 0 && std::abs(std::fma(dir.x, -t3, pmu.x)) <= l1 && std::abs(std::fma(dir.y, -t3, pmu.y)) <= l2) ?
        t3 : infinity;
}

/* Returns the barycentric info (the faces behave like quads) [to be implemented] */
barycentric_info box::get_barycentric(const rt::vector&) const {

    return barycentric_info(0.0_r, 0.0_r, object_type::Box);
}

rt::vector box::compute_normal_from_map(
            const rt::vector&,
            const rt::vector&,
            const texture_info&
        ) const {

    throw std::runtime_error("Texturing is unavailable for boxes");
}

rt::vector box::sample(const randomgen&) const {
    throw std::runtime_error("Sampling is unavailable for boxes");
}

rt::vector box::sample_visible(const randomgen&, const rt::vector&) const {
    throw std::runtime_error("Sampling is unavailable for boxes");
}