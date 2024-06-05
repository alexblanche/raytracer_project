#include "scene/objects/box.hpp"

#include <optional>

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"


box::box() : object(), n1(rt::vector(1,0,0)), n2(rt::vector(0,1,0)), n3(rt::vector(0,0,1)), l1(100), l2(100), l3(100) {}
        
/* The vector n3 is taken as the cross product of n1 and n2 */
box::box(const rt::vector& center, const rt::vector& n1, const rt::vector& n2,
            const real& l1, const real& l2, const real& l3, const size_t material_index)
    
    : object(center, material_index), n1(n1), n2(n2), n3(n1 ^ n2), l1(l1/2), l2(l2/2), l3(l3/2) {}

/* The vector n3 is taken as the cross product of n1 and n2 */
box::box(const rt::vector& center, const rt::vector& n1, const rt::vector& n2,
            const real& l1, const real& l2, const real& l3)
    
    : object(center, (size_t) (-1)), n1(n1), n2(n2), n3(n1 ^ n2), l1(l1/2), l2(l2/2), l3(l3/2) {}



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

    const rt::vector dir = r.get_direction();

    /** This whole function to be optimized (it will then become incomprehensible)
     * 
     * const vector pmu = position - u;
     * t1 = ((c1 - u) | n1) / pdt1
     *      = ((position - (l1 * pdt1 / abs(pdt1)) * n1 - u) | n1) / pdt1
     *      = ((pmu | n1) - l1 * pdt1 / abs(pdt1)) / pdt1
     *      = ((pmu | n1) / pdt1 - l1 / abs(pdt1))
     *      = pmun1 / pdt1 - l1 / abspdt1
     * 
     * (p | n2)
     *      = ((c1 - u - (t1*dir)) | n2)
     *      = ((position - (...)*n1 - u - t1*dir) | n2)
     *      = (pmu | n2) - t1 * pdt2
     *      = pmnu2 - t1 * pdt2
     * **/

    const rt::vector pmu = position - r.get_origin();
    const real pmun1 = (pmu | n1);
    const real pmun2 = (pmu | n2);
    const real pmun3 = (pmu | n3);

    // Factor that depends on whether u is outside or inside the box
    const real a = (abs(pmun1) <= l1 && abs(pmun2) <= l2 && abs(pmun3) <= l3) ?
        /* inside */ 1.0f :
        /* outside */ -1.0f;

    const real pdt1 = (dir | n1);
    const real pdt2 = (dir | n2);
    const real pdt3 = (dir | n3);

    const real abspdt1 = abs(pdt1);
    if (abspdt1 > 0.0000001f) {
        // Determination of t1
        const real t1 = pmun1 / pdt1 + a * l1 / abspdt1;
        // Check that t1 gives a point inside the face
        if (abs(pmun2 - t1 * pdt2) <= l2 && abs(pmun3 - t1 * pdt3) <= l3) {
            if (t1 >= 0.0f) {
                return t1;
            }
            else {
                return std::nullopt;
            }
        }
    }

    const real abspdt2 = abs(pdt2);
    if (abspdt2 > 0.0000001f) {
        const real t2 = pmun2 / pdt2 + a * l2 / abspdt2;
        if (abs(pmun1 - t2 * pdt1) <= l1 && abs(pmun3 - t2 * pdt3) <= l3) {
            if (t2 >= 0.0f) {
                return t2;
            }
            else {
                return std::nullopt;
            }
        }
    }

    const real abspdt3 = abs(pdt3);
    if (abspdt3 > 0.0000001f) {
        const real t3 = pmun3 / pdt3 + a * l3 / abspdt3;
        if (t3 >= 0.0f && abs(pmun1 - t3 * pdt1) <= l1 && abs(pmun2 - t3 * pdt2) <= l2) {
            return t3;
        }
    }

    return std::nullopt;
}
    /** Original version */
    /*
    if (abs(pdt1) > 0.0000001f) {
        // Determination of t1
        const rt::vector c1 = position - (l1 * pdt1 / abs(pdt1)) * n1;
        t1 = ((c1 - u) | n1) / pdt1;

        // Check that t1 gives a point inside the face
        const rt::vector p = c1 - u - (t1 * dir);
        const real checkpdt2 = (p | n2);
        if (abs(checkpdt2) <= l2) {
            const real checkpdt3 = (p | n3);
            if (abs(checkpdt3) <= l3) {
                t1 = infinity;
            }
        }
        else {
            t1 = infinity;
        }

        if (t1 < 0) { return infinity; }
    }

    if (abs(pdt2) > 0.0000001f) {
        const rt::vector c2 = position - (l2 * pdt2/ abs(pdt2)) * n2;
        t2 = ((c2 - u) | n2) / pdt2;
    
        const rt::vector p = c2 - u - (t2 * dir);
        const real checkpdt1 = (p | n1);
        if (abs(checkpdt1) <= l1) {
            const real checkpdt3 = (p | n3);
            if (abs(checkpdt3) <= l3) {
                t2 = infinity;
            }
        }
        else {
            t2 = infinity;
        }

        if (t2 < 0) { return infinity; }
    }

    if (abs(pdt3) > 0.0000001f) {
        const rt::vector c3 = position - (l3 * pdt3/ abs(pdt3)) * n3;
        t3 = ((c3 - u) | n3) / pdt3;
        
        const rt::vector p = c3 - u - (t3 * dir);
        const real checkpdt1 = (p | n1);
        if (abs(checkpdt1) <= l1) {
            const real checkpdt2 = (p | n2);
            if (abs(checkpdt2) <= l2) {
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
        
hit box::compute_intersection(ray& r, const real& t) const {
    // Intersection point
    const rt::vector u = r.get_origin();
    const rt::vector p = u + t * r.get_direction();

    // Re-computing the face of intersection
    // (not great, but the alternative is to return a hit object (with distance t) for every intersection check)

    // Shifting the position a little bit, to avoid the ray hitting the object itself again
    const rt::vector v = p - position;
    const object* pt_obj = this;
    ray* pt_ray = &r;

    const real pdt1 = (v | n1);
    if (abs(pdt1 - l1) < 0.0000001f) {
        return hit(pt_ray, p, n1, pt_obj);
    }
    else if (abs(pdt1 + l1) < 0.0000001f) {
        return hit(pt_ray, p, (-1)*n1, pt_obj);
    }
    else {
        const real pdt2 = (v | n2);
        if (abs(pdt2 - l2) < 0.0000001f) {
            return hit(pt_ray, p, n2, pt_obj);
        }
        else if (abs(pdt2 + l2) < 0.0000001f) {
            return hit(pt_ray, p, (-1)*n2, pt_obj);
        }
        else {
            const real pdt3 = (v | n3);
            if (abs(pdt3 - l3) < 0.0000001f) {
                return hit(pt_ray, p, n3, pt_obj);
            }
            else {
                return hit(pt_ray, p, (-1)*n3, pt_obj);
            } 
        }   
    }
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

    return min_max_coord(min_x, max_x, min_y, max_y, min_z, max_z);
}


/* Specific to (standard) boxes: returns true if the ray r hits the box
   The box is assumed to be standard (axes are n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1)) */
bool box::is_hit_by(const ray& r) const {
    
    const rt::vector dir = r.get_direction();
    const rt::vector inv_dir = r.get_inv_dir();
    const rt::vector abs_inv_dir = r.get_abs_inv_dir();

    // See measure_distance

    const rt::vector u = r.get_origin();
    const real pmux = position.x - u.x;
    const real pmuy = position.y - u.y;
    const real pmuz = position.z - u.z;
    
    if (abs(pmux) <= l1 && abs(pmuy) <= l2 && abs(pmuz) <= l3) {
        // u is inside the box
        return true;
    }

    if (dir.x != 0.0f) {
        // Determination of t1
        const real t1 = pmux * inv_dir.x - l1 * abs_inv_dir.x;
        // Check that t1 gives a point inside the face
        if (abs(pmuy - t1 * dir.y) <= l2 && abs(pmuz - t1 * dir.z) <= l3) {
            return (t1 >= 0.0f);
        }
    }

    if (dir.y != 0.0f) {
        const real t2 = pmuy * inv_dir.y - l2 * abs_inv_dir.y;
        if (abs(pmux - t2 * dir.x) <= l1 && abs(pmuz - t2 * dir.z) <= l3) {
            return (t2 >= 0.0f);
        }
    }

    if (dir.z != 0.0f) {
        const real t3 = pmuz * inv_dir.z - l3 * abs_inv_dir.z;
        return (t3 >= 0.0f && abs(pmux - t3 * dir.x) <= l1 && abs(pmuy - t3 * dir.y) <= l2);
    }

    return false;
}