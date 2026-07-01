#include "scene/objects/cylinder.hpp"
#include "light/vector.hpp"
#include "scene/material/material.hpp"
#include "auxiliary/utils.hpp"

#include <cmath>
#include <stdexcept>
#include <optional>


cylinder::cylinder(const rt::vector& origin, const rt::vector& direction,
    const real radius, const real length, const unsigned int material_index)

    : object(origin, material_index),
        direction(direction), radius(radius), length(length) {}




/* Intersection determination */

/* Calculates and returns the intersection value t */
real cylinder::measure_distance(const ray& r) const {

    /* We denote the origin, direction and radius of the cylinder o, d and r,
       and the origin and direction of the ray. */

    /* Step 1: check if the ray intersects the side of the cylinder

       When p is a point in space, the projection of p on the line of vector d is
       (o + s.d), where s is such that (p - (o + sd) | d) = 0,
       i.e. (p - o | d) - s (d | d) = 0, hence s = (p - o | d)
       When p = u + t dir, s = (u + t dir - o | d) = (u - o | d) + t (dir | d)
       
       We search for t such that (u + t dir - (o + ((u-o|d) + t (dir|d)) d)).norm() = r,
       so ((u - o - (u-o|d)d) + t (dir - (dir|d)d)).normsq() - r^2 = 0,
       If we write A = (u - o - (u-o|d)d) = (Ax, Ay, Az) and B = (dir - (dir|d)d) = (Bx, By, Bz),
       we can rewrite the expression as:
       (A + t B).normsq() - r^2 = 0
       (Ax + t Bx)^2 + (Ay + t By)^2 + (Az + t Bz)^2 - r^2 = 0,
       Ax^2 + 2tAxBx + t^2 Bx^2 + ...(y)... + ...(z)... - r^2 = 0,
       t^2(Bx^2 + By^2 + Bz^2) + 2t(AxBx + AyBy + AzBz) + (Ax^2 + Ay^2 + Az^2 - r^2) = 0,
       t^2 * B.normsq() + 2t * (A|B) + (A.normsq() - r^2) = 0,

       We solve for t: delta = 4(A|B)^2 - 4*B.normsq()*(A.normsq() - r^2).
       If delta >= 0, t = - (A|B) +- sqrt(delta/4) / B.normsq(),
       t is a solution if (1) t >= 0, (2) 0 <= s <= length, i.e.
       0 <= (u - o | d) + t (dir | d) <= length

       The case analysis is detailed below.
    */

    const rt::vector ump = r.origin - position;
    const real umpdirec = (ump | direction);
    const real dirdirec = (r.direction | direction);
    const rt::vector a = ump - (umpdirec * direction);
    const rt::vector b = r.direction - (dirdirec * direction);

    const real ab = (a | b);
    const real bb = b.normsq();
    const real rr = radius * radius;
    /* delta is actually the discriminant / 4 */
    const real delta = ab * ab - bb * (a.normsq() - rr);

    if (is_negative(delta)) {
        // The ray does not intersect the infinite cylinder
        return infinity;
    }

    const real sqrtdelta = sqrt(delta);
    
    const real t1 = (- ab - sqrtdelta) / bb;
    bool outside = true;
    
    if (is_positive(t1)) {
        const real s1 = umpdirec + t1 * dirdirec;
        if (is_positive(s1)) {
            if (s1 <= length) {
                return t1;
            }
            else {
                const real t2 = (- ab + sqrtdelta) / bb;
                const real s2 = umpdirec + t2 * dirdirec;
                if (s2 > length) { return infinity; }
                // else: the ray goes through one or both edge disks
            }
        }
        else {
            const real t2 = (- ab + sqrtdelta) / bb;
            const real s2 = umpdirec + t2 * dirdirec;
            if (is_negative_not_zero(s2)) { return infinity; }
            // else: the ray goes through one or both edge disks
        }
    }
    else {
        const real t2 = (- ab + sqrtdelta) / bb;
        if (is_positive(t2)) {
            /* Case analysis:
               We compute s1 (associated with t1), s2 and whether we are outside the cylinder.
               "s ok" means that 0 <= s <= length, i.e. the projection on the line is within the cylinder.

               * if (s1 not ok) && (s2 not ok) ->
                    if (s1 < 0 && s2 < 0 || s1 > length && s2 > length): return infinity (the ray misses the cylinder)
                    else: continue (if outside, the ray goes through both edge disks, otherwise only one edge disk)
               * if (s1 ok) && (s2 ok) -> return t2 (u is inside the cylinder)
               * if (s1 not ok) && (s2 ok) ->
                    if outside: continue (the edge disk is between u and the intersection at t2)
                    else: return t2 (u is inside, so the intersection is at t2)
               * if (s1 ok) && (s2 not ok) ->
                    if outside: return infinity (u is outside and the cylinder is behind it)
                    else: continue (the edge disk is between u and the intersection with the infinite cylinder at t2) 
            */

            const real s1 = umpdirec + t1 * dirdirec;
            const real s2 = umpdirec + t2 * dirdirec;
            const bool s1ok = s1 >= 0.0_r && s1 <= length;
            const bool s2ok = s2 >= 0.0_r && s2 <= length;
            outside = is_negative_not_zero(umpdirec) || umpdirec > length;
            if (s2ok) {
                if (s1ok || (not outside)) { return t2; }
                // else: the edge disk is between u and the intersection at t2
            }
            else {
                // if ((s1ok && outside) || (s1 < 0 && s2 < 0) || (s1 > length && s2 > length)) {
                //     return infinity;
                // }
                if (is_negative_not_zero(s1)) {
                    if (is_negative_not_zero(s2)) { return infinity; }
                }
                else {
                    if (s1 <= length) {
                        if (outside) { return infinity; }
                    }
                    else {
                        if (s2 > length) { return infinity; }
                    }
                }
            }
        }
        else {
            // The cylinder is behind the ray
            return infinity;
        }
    }

    /* Step 2: if a solution has not been found, check the two edge disks
    
       Now let us compute the intersection with the disk at the edge of the cylinder.
       Its center is denoted v:
       if we are outside the infinite cylinder,
           if (dir | d) >= 0, v = o,
           otherwise v = o + length * d
       if we are inside the infinite cylinder,
           if (dir | d) >= 0, v = o + length * d,
           otherwise v = o

       We compute t such that u + t dir belongs to the plane orthogonal with d and located at v:
       (u + t dir - v | d) = 0,
       t = (v - u | d) / (dir | d)
       (if (dir|d) != 0, which we may assume, since otherwise we would have concluded at step 1)
       t is a solution if t >= 0 and (u + t dir - v).normsq() <= r^2, which we may also assume.
    */

    // We may assume that (dir | direction) != 0, since otherwise we would have concluded at step 1

    // if (outside == (dirdirec >= 0.0_r)) {
    //     // v = position
    //     // t = ((v - u) | direction) / (dir | direction)
    //     return - umpdirec / dirdirec;
    // }
    // else {
    //     // v = position + length * direction
    //     // t = ((v - u) | direction) / (dir | direction) 
    //     //   = (-umpdirec + length (direction | direction)) / dirdirec
    //     return (- umpdirec + length) / dirdirec;
    // }
    return (outside == (dirdirec >= 0.0_r)) ?
        - umpdirec / dirdirec
        :
        (- umpdirec + length) / dirdirec;
}

/* Returns the hit corresponding with the given intersection value t */
hit cylinder::compute_intersection(const ray& r, const real t) const {

    // Intersection point
    const rt::vector p = r.extend(t);
    const real rr = radius * radius;
    const rt::vector pmpos = p - position;

    const real not_on_bottom_disk = pmpos.normsq() >= rr;
    const bool hits_side = not_on_bottom_disk
        && /* Not on top disk */
        ((pmpos - (length * direction)).normsq() >= rr);
    
    const object* pt_obj = this;
    const ray* pt_ray = &r;

    if (hits_side) {
        // We compute the s value (such that (p - (o + s.d) | d) = 0)
        // const real s = (pmpos | direction);
        // const rt::vector n = (p - (position + s * d)) / radius
        return hit(pt_ray, p, (pmpos - ((pmpos | direction) * direction)) / radius, pt_obj);
    }
    else {
        return hit(pt_ray, p, not_on_bottom_disk ? direction : ((-1) * direction), pt_obj);
    }

    
}

/* Minimum and maximum coordinates */
min_max_coord cylinder::get_min_max_coord() const {

    /* Exact solution is not trivial, an upper bound will do */

    const rt::vector lendir = length * direction;
    const rt::vector rv(radius, radius, radius);

    const auto [ min_l, max_l ] = rt::min_max(lendir, ZERO);

    const rt::vector max = position + rv + max_l;
    const rt::vector min = position - rv + min_l;

    return build_min_max_coord(min, max);
}

/* Returns the barycentric info (the texture is mapped onto the top, the bottom, and the curved surface) [to be implemented] */
barycentric_info cylinder::get_barycentric(const rt::vector&) const {
    throw std::runtime_error("Barycentric coordinates are unavailable for cylinders");
}

rt::vector cylinder::compute_normal_from_map(
            const rt::vector&,
            const rt::vector&,
            const texture_info&
        ) const {

    throw std::runtime_error("Texturing is unavailable for cylinders");
}

rt::vector cylinder::sample(const randomgen&) const {
    throw std::runtime_error("Sampling is unavailable for cylinders");
}

rt::vector cylinder::sample_visible(const randomgen&, const rt::vector&) const {
    throw std::runtime_error("Sampling is unavailable for cylinders");
}