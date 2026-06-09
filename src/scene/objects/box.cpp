#include "scene/objects/box.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"
#include "auxiliary/utils.hpp"

#include <optional>
#include <stdexcept>


box::box() : object(), n1(rt::vector(1,0,0)), n2(rt::vector(0,1,0)), n3(rt::vector(0,0,1)), l1(100), l2(100), l3(100) {}
        
/* The vector n3 is taken as the cross product of n1 and n2 */
box::box(const rt::vector& center, const rt::vector& n1, const rt::vector& n2,
            const real l1, const real l2, const real l3, const unsigned int material_index)
    
    : object(center, material_index), n1(n1), n2(n2), n3(n1 ^ n2), l1(l1/2), l2(l2/2), l3(l3/2) {}

/* The vector n3 is taken as the cross product of n1 and n2 */
box::box(const rt::vector& center, const rt::vector& n1, const rt::vector& n2,
            const real l1, const real l2, const real l3)
    
    : object(center, EMPTY_INDEX), n1(n1), n2(n2), n3(n1 ^ n2), l1(l1/2), l2(l2/2), l3(l3/2) {}



/* Intersection determination */

std::optional<real> box::measure_distance(const ray& r) const {
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

    /** This whole function to be optimized (it will then become incomprehensible)
     * 
     * const vector pmu = position - u;
     * t1 = ((c1 - u) | n1) / pdt1
     *      = ((position - (l1 * pdt1 / std::abs(pdt1)) * n1 - u) | n1) / pdt1
     *      = ((pmu | n1) - l1 * pdt1 / std::abs(pdt1)) / pdt1
     *      = ((pmu | n1) / pdt1 - l1 / std::abs(pdt1))
     *      = pmun1 / pdt1 - l1 / abspdt1
     * 
     * (p | n2)
     *      = ((c1 - u - (t1*dir)) | n2)
     *      = ((position - (...)*n1 - u - t1*dir) | n2)
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
        if (std::abs(pmun2 - t1 * pdt2) <= l2 && std::abs(pmun3 - t1 * pdt3) <= l3) {
            return (is_positive(t1)) ?
                  std::optional(t1)
                : std::nullopt;
        }
    }

    if (is_not_zero(pdt2)) {
        const real t2 = pmun2 / pdt2 + a * l2 / std::abs(pdt2);
        if (std::abs(pmun1 - t2 * pdt1) <= l1 && std::abs(pmun3 - t2 * pdt3) <= l3) {
            return (is_positive(t2)) ?
                  std::optional(t2)
                : std::nullopt;
        }
    }

    if (is_not_zero(pdt3)) {
        const real t3 = pmun3 / pdt3 + a * l3 / std::abs(pdt3);
        return (is_positive(t3) && std::abs(pmun1 - t3 * pdt1) <= l1 && std::abs(pmun2 - t3 * pdt2) <= l2) ?
              std::optional(t3)
            : std::nullopt;
    }

    return std::nullopt;
}
    /** Original version */
    /*
    if (std::abs(pdt1) > 0.0000001f) {
        // Determination of t1
        const rt::vector c1 = position - (l1 * pdt1 / std::abs(pdt1)) * n1;
        t1 = ((c1 - u) | n1) / pdt1;

        // Check that t1 gives a point inside the face
        const rt::vector p = c1 - u - (t1 * dir);
        const real checkpdt2 = (p | n2);
        if (std::abs(checkpdt2) <= l2) {
            const real checkpdt3 = (p | n3);
            if (std::abs(checkpdt3) <= l3) {
                t1 = infinity;
            }
        }
        else {
            t1 = infinity;
        }

        if (t1 < 0) { return infinity; }
    }

    if (std::abs(pdt2) > 0.0000001f) {
        const rt::vector c2 = position - (l2 * pdt2/ std::abs(pdt2)) * n2;
        t2 = ((c2 - u) | n2) / pdt2;
    
        const rt::vector p = c2 - u - (t2 * dir);
        const real checkpdt1 = (p | n1);
        if (std::abs(checkpdt1) <= l1) {
            const real checkpdt3 = (p | n3);
            if (std::abs(checkpdt3) <= l3) {
                t2 = infinity;
            }
        }
        else {
            t2 = infinity;
        }

        if (t2 < 0) { return infinity; }
    }

    if (std::abs(pdt3) > 0.0000001f) {
        const rt::vector c3 = position - (l3 * pdt3/ std::abs(pdt3)) * n3;
        t3 = ((c3 - u) | n3) / pdt3;
        
        const rt::vector p = c3 - u - (t3 * dir);
        const real checkpdt1 = (p | n1);
        if (std::abs(checkpdt1) <= l1) {
            const real checkpdt2 = (p | n2);
            if (std::abs(checkpdt2) <= l2) {
                t3 = infinity;
            }
        }
        else {
            t3 = infinity;
        }

        if (t3 < 0) { return infinity; }
    }
    
    return std::min(t1, std::min(t2, t3));
    */
        
hit box::compute_intersection(const ray& r, const real t) const {
    // Intersection point
    const rt::vector& u = r.origin;
    const rt::vector p = fma(r.direction, t, u);

    // Re-computing the face of intersection
    // (not great, but the alternative is to return a hit object (with distance t) for every intersection check)

    // Shifting the position a little bit, to avoid the ray hitting the object itself again
    const rt::vector v = p - position;
    const object* pt_obj = this;
    const ray* pt_ray = &r;

    const real pdt1 = (v | n1);
    if (std::abs(pdt1 - l1) < 0.0000001_r)
        return hit(pt_ray, p, n1, pt_obj);
    
    if (std::abs(pdt1 + l1) < 0.0000001_r)
        return hit(pt_ray, p, (-1.0_r) * n1, pt_obj);
    
    const real pdt2 = (v | n2);
    if (std::abs(pdt2 - l2) < 0.0000001_r)
        return hit(pt_ray, p, n2, pt_obj);
    
    if (std::abs(pdt2 + l2) < 0.0000001_r)
        return hit(pt_ray, p, (-1.0_r) * n2, pt_obj);
    
    const real pdt3 = (v | n3);
    if (std::abs(pdt3 - l3) < 0.0000001_r)
        return hit(pt_ray, p, n3, pt_obj);

    return hit(pt_ray, p, (-1.0_r) * n3, pt_obj);
}

/* Minimum and maximum coordinates */
min_max_coord box::get_min_max_coord() const {

    // (n1 * a1x) has a positive .x, (n1 * (-a1x)) has a negative one
    const real a1x = n1.x >= 0 ? 1 : (-1);
    const real a2x = n2.x >= 0 ? 1 : (-1);
    const real a3x = n3.x >= 0 ? 1 : (-1);

    const real max_x = (position + l1 * (a1x * n1) + l2 * (a2x * n2) + l3 * (a3x * n3)).x;
    const real min_x = (position + l1 * ((- a1x) * n1) + l2 * ((- a2x) * n2) + l3 * ((- a3x) * n3)).x;

    const real a1y = n1.y >= 0 ? 1 : (-1);
    const real a2y = n2.y >= 0 ? 1 : (-1);
    const real a3y = n3.y >= 0 ? 1 : (-1);

    const real max_y = (position + l1 * (a1y * n1) + l2 * (a2y * n2) + l3 * (a3y * n3)).y;
    const real min_y = (position + l1 * ((- a1y) * n1) + l2 * ((- a2y) * n2) + l3 * ((- a3y) * n3)).y;

    const real a1z = n1.z >= 0 ? 1 : (-1);
    const real a2z = n2.z >= 0 ? 1 : (-1);
    const real a3z = n3.z >= 0 ? 1 : (-1);

    const real max_z = (position + l1 * (a1z * n1) + l2 * (a2z * n2) + l3 * (a3z * n3)).z;
    const real min_z = (position + l1 * ((- a1z) * n1) + l2 * ((- a2z) * n2) + l3 * ((- a3z) * n3)).z;

    return { min_x, max_x, min_y, max_y, min_z, max_z };
}


/* Specific to (standard) boxes: returns true if the ray r hits the box
   The box is assumed to be standard (axes are n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1)) */
bool box::is_hit_by(const ray& r) const {
    
    const auto& [ u, dir, inv_dir, abs_inv_dir] = r;

    // See measure_distance

    const rt::vector pmu = position - u;
    
    if (std::abs(pmu.x) <= l1 && std::abs(pmu.y) <= l2 && std::abs(pmu.z) <= l3) {
        // u is inside the box
        return true;
    }

    if (is_not_zero(dir.x)) {
        // Determination of t1
        const real t1 = pmu.x * inv_dir.x - l1 * abs_inv_dir.x;
        // Check that t1 gives a point inside the face
        if (std::abs(pmu.y - t1 * dir.y) <= l2 && std::abs(pmu.z - t1 * dir.z) <= l3) {
            return is_positive(t1);
        }
    }

    if (is_not_zero(dir.y)) {
        const real t2 = pmu.y * inv_dir.y - l2 * abs_inv_dir.y;
        if (std::abs(pmu.x - t2 * dir.x) <= l1 && std::abs(pmu.z - t2 * dir.z) <= l3) {
            return is_positive(t2);
        }
    }

    if (is_not_zero(dir.z)) {
        const real t3 = pmu.z * inv_dir.z - l3 * abs_inv_dir.z;
        return (is_positive(t3) && std::abs(pmu.x - t3 * dir.x) <= l1 && std::abs(pmu.y - t3 * dir.y) <= l2);
    }

    return false;
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

rt::vector box::sample(randomgen&) const {
    throw std::runtime_error("Sampling is unavailable for boxes");
}

rt::vector box::sample_visible(randomgen&, const rt::vector&) const {
    throw std::runtime_error("Sampling is unavailable for boxes");
}