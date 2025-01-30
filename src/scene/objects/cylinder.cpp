#include "scene/objects/cylinder.hpp"
#include "light/vector.hpp"
#include "scene/material/material.hpp"
#include <math.h>

#include <optional>


/* Constructors */

cylinder::cylinder()
    : radius(0), length(0) {}

cylinder::cylinder(const rt::vector& origin, const rt::vector& direction,
    const real radius, const real length, const size_t material_index)

    : object(origin, material_index),
        direction(direction), radius(radius), length(length) {}




/* Intersection determination */

/* Calculates and returns the intersection value t */
std::optional<real> cylinder::measure_distance(const ray& r) const {

    /* We denote the origin, direction and radius of the cylinder o, d and r,
       and the origin and direction of the ray u and dir. */

    const rt::vector& u = r.get_origin();
    const rt::vector& dir = r.get_direction();

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

    const rt::vector ump = u - position;
    const real umpdirec = (ump | direction);
    const real dirdirec = (dir | direction);
    const rt::vector a = ump - (umpdirec * direction);
    const rt::vector b = dir - (dirdirec * direction);

    const real ab = (a | b);
    const real bb = b.normsq();
    const real rr = radius * radius;
    /* delta is actually the discriminant / 4 */
    const real delta = ab*ab - bb*(a.normsq() - rr);

    if (delta < 0) {
        // The ray does not intersect the infinite cylinder
        return std::nullopt;
    }

    const real sqrtdelta = sqrt(delta);
    
    const real t1 = (- ab - sqrtdelta) / bb;
    bool outside = true;
    
    if (t1 >= 0.0f) {
        const real s1 = umpdirec + t1 * dirdirec;
        if (s1 >= 0.0f) {
            if (s1 <= length) {
                return t1;
            }
            else {
                const real t2 = (- ab + sqrtdelta) / bb;
                const real s2 = umpdirec + t2 * dirdirec;
                if (s2 > length) { return std::nullopt; }
                // else: the ray goes through one or both edge disks
            }
        }
        else {
            const real t2 = (- ab + sqrtdelta) / bb;
            const real s2 = umpdirec + t2 * dirdirec;
            if (s2 < 0.0f) { return std::nullopt; }
            // else: the ray goes through one or both edge disks
        }
    }
    else {
        const real t2 = (- ab + sqrtdelta) / bb;
        if (t2 >= 0.0f) {
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
            const bool s1ok = s1 >= 0.0f && s1 <= length;
            const bool s2ok = s2 >= 0.0f && s2 <= length;
            outside = umpdirec < 0.0f || umpdirec > length;
            if (s2ok) {
                if (s1ok || (not outside)) { return t2; }
                // else: the edge disk is between u and the intersection at t2
            }
            else {
                // if ((s1ok && outside) || (s1 < 0 && s2 < 0) || (s1 > length && s2 > length)) {
                //     return infinity;
                // }
                if (s1 < 0.0f) {
                    if (s2 < 0.0f) { return std::nullopt; }
                }
                else {
                    if (s1 <= length) {
                        if (outside) { return std::nullopt; }
                    }
                    else {
                        if (s2 > length) { return std::nullopt; }
                    }
                }
            }
        }
        else {
            // The cylinder is behind the ray
            return std::nullopt;
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

    if (outside == (dirdirec >= 0.0f)) {
        // v = position
        // t = ((v - u) | direction) / (dir | direction)
        return - umpdirec / dirdirec;
    }
    else {
        // v = position + length * direction
        // t = ((v - u) | direction) / (dir | direction) 
        //   = (-umpdirec + length (direction | direction)) / dirdirec
        return (- umpdirec + length) / dirdirec;
    }
}

/* Returns the hit corresponding with the given intersection value t */
hit cylinder::compute_intersection(ray& r, const real t) const {

    // Intersection point
    const rt::vector& u = r.get_origin();
    const rt::vector& dir = r.get_direction();
    const rt::vector p = u + t * dir;
    const real rr = radius * radius;
    const rt::vector pmpos = p - position;

    const real not_on_bottom_disk = pmpos.normsq() >= rr;
    const bool hits_side = not_on_bottom_disk
        && /* Not on top disk */
        ((pmpos - (length * direction)).normsq() >= rr);
    
    const object* pt_obj = this;
    ray* pt_ray = &r;

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

    real min_x, max_x, min_y, max_y, min_z, max_z;
    
    /* Exact solution is not trivial, so I leave a upper bound */
    if (direction.x >= 0.0f) {
        max_x = position.x + length * direction.x + radius;
        min_x = position.x - radius;
    }
    else {
        max_x = position.x + radius;
        min_x = position.x + length * direction.x - radius;
    }

    if (direction.y >= 0.0f) {
        max_y = position.y + direction.y + radius;
        min_y = position.y - radius;
    }
    else {
        max_y = position.y + radius;
        min_y = position.y + length * direction.y - radius;
    }

    if (direction.z >= 0.0f) {
        max_z = position.z + length * direction.z + radius;
        min_z = position.z - radius;
    }
    else {
        max_z = position.z + radius;
        min_z = position.z + length * direction.z - radius;
    }

    return min_max_coord(min_x, max_x, min_y, max_y, min_z, max_z);
}