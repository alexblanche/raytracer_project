#include "scene/objects/box.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"

#include<limits>
numeric_limits<double> realbx;
const double infinity = realbx.infinity();


box::box() : object(), n1(rt::vector(1,0,0)), n2(rt::vector(0,1,0)), n3(rt::vector(0,0,1)), l1(100), l2(100), l3(100) {}
        
/* The vector n3 is taken as the cross product of n1 and n2 */
box::box(const rt::vector& center, const rt::vector& n1, const rt::vector& n2,
            const double l1, const double l2, const double l3, const material& material)
    
    : object(center, material), n1(n1), n2(n2), n3(n1 ^ n2), l1(l1/2), l2(l2/2), l3(l3/2) {}

/* The vector n3 is taken as the cross product of n1 and n2 */
box::box(const rt::vector& center, const rt::vector& n1, const rt::vector& n2,
            const double l1, const double l2, const double l3)
    
    : object(center, material()), n1(n1), n2(n2), n3(n1 ^ n2), l1(l1/2), l2(l2/2), l3(l3/2) {}



/* Intersection determination */

double box::measure_distance(const ray& r) const {
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
    const double pmun1 = (pmu | n1);
    const double pmun2 = (pmu | n2);
    const double pmun3 = (pmu | n3);

    // Factor that depends on whether u is outside or inside the box
    const double a = (abs(pmun1) <= l1 && abs(pmun2) <= l2 && abs(pmun3) <= l3) ?
        /* inside */ 1 :
        /* outside */ -1;

    const double pdt1 = (dir | n1);
    const double pdt2 = (dir | n2);
    const double pdt3 = (dir | n3);

    const double abspdt1 = abs(pdt1);
    if (abspdt1 > 0.0000001) {
        // Determination of t1
        const double t1 = pmun1 / pdt1 + a * l1 / abspdt1;
        // Check that t1 gives a point inside the face
        if (abs(pmun2 - t1 * pdt2) <= l2 && abs(pmun3 - t1 * pdt3) <= l3) {
            return t1 >= 0 ? t1 : infinity;
        }
    }

    const double abspdt2 = abs(pdt2);
    if (abspdt2 > 0.0000001) {
        const double t2 = pmun2 / pdt2 + a * l2 / abspdt2;
        if (abs(pmun1 - t2 * pdt1) <= l1 && abs(pmun3 - t2 * pdt3) <= l3) {
            return t2 >= 0 ? t2 : infinity;
        }
    }

    const double abspdt3 = abs(pdt3);
    if (abspdt3 > 0.0000001) {
        const double t3 = pmun3 / pdt3 + a * l3 / abspdt3;
        if (t3 >= 0 && abs(pmun1 - t3 * pdt1) <= l1 && abs(pmun2 - t3 * pdt2) <= l2) {
            return t3;
        }
    }

    return infinity;

    /** Original version */
    /*
    if (abs(pdt1) > 0.0000001) {
        // Determination of t1
        const rt::vector c1 = position - (l1 * pdt1 / abs(pdt1)) * n1;
        t1 = ((c1 - u) | n1) / pdt1;

        // Check that t1 gives a point inside the face
        const rt::vector p = c1 - u - (t1 * dir);
        const double checkpdt2 = (p | n2);
        if (abs(checkpdt2) <= l2) {
            const double checkpdt3 = (p | n3);
            if (abs(checkpdt3) <= l3) {
                t1 = infinity;
            }
        }
        else {
            t1 = infinity;
        }

        if (t1 < 0) { return infinity; }
    }

    if (abs(pdt2) > 0.0000001) {
        const rt::vector c2 = position - (l2 * pdt2/ abs(pdt2)) * n2;
        t2 = ((c2 - u) | n2) / pdt2;
    
        const rt::vector p = c2 - u - (t2 * dir);
        const double checkpdt1 = (p | n1);
        if (abs(checkpdt1) <= l1) {
            const double checkpdt3 = (p | n3);
            if (abs(checkpdt3) <= l3) {
                t2 = infinity;
            }
        }
        else {
            t2 = infinity;
        }

        if (t2 < 0) { return infinity; }
    }

    if (abs(pdt3) > 0.0000001) {
        const rt::vector c3 = position - (l3 * pdt3/ abs(pdt3)) * n3;
        t3 = ((c3 - u) | n3) / pdt3;
        
        const rt::vector p = c3 - u - (t3 * dir);
        const double checkpdt1 = (p | n1);
        if (abs(checkpdt1) <= l1) {
            const double checkpdt2 = (p | n2);
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
}
        
hit box::compute_intersection(const ray& r, const double t) const {
    // Intersection point
    const rt::vector& u = r.get_origin();
    const rt::vector p = u + t * r.get_direction();

    // Re-computing the face of intersection
    // (not great, but the alternative is to return a hit object (with distance t) for every intersection check)

    // Shifting the position a little bit, to avoid the ray hitting the object itself again
    const rt::vector v = p - position;
    const object* pt = this;

    const double pdt1 = (v | n1);
    if (abs(pdt1 - l1) < 0.0000001) {
        return hit(r, p, n1, pt);
    }
    else if (abs(pdt1 + l1) < 0.0000001) {
        return hit(r, p, (-1)*n1, pt);
    }
    else {
        const double pdt2 = (v | n2);
        if (abs(pdt2 - l2) < 0.0000001) {
            return hit(r, p, n2, pt);
        }
        else if (abs(pdt2 + l2) < 0.0000001) {
            return hit(r, p,(-1)*n2, pt);
        }
        else {
            const double pdt3 = (v | n3);
            if (abs(pdt3 - l3) < 0.0000001) {
                return hit(r, p, n3, pt);
            }
            else {
                return hit(r, p, (-1)*n3, pt);
            } 
        }   
    }
}

/* Minimum and maximum coordinates */
void box::min_max_coord(double& min_x, double& max_x,
    double& min_y, double& max_y, double& min_z, double& max_z) const {

    // (n1 * a1x) has a positive .x, (n1 * (-a1x)) has a negative one
    const double a1x = n1.x >= 0 ? 1 : (-1);
    const double a2x = n2.x >= 0 ? 1 : (-1);
    const double a3x = n3.x >= 0 ? 1 : (-1);

    max_x = (position + (a1x * n1) + (a2x * n2) + (a3x * n3)).x;
    min_x = (position + ((- a1x) * n1) + ((- a2x) * n2) + ((- a3x) * n3)).x;

    const double a1y = n1.y >= 0 ? 1 : (-1);
    const double a2y = n2.y >= 0 ? 1 : (-1);
    const double a3y = n3.y >= 0 ? 1 : (-1);

    max_y = (position + (a1y * n1) + (a2y * n2) + (a3y * n3)).y;
    min_y = (position + ((- a1y) * n1) + ((- a2y) * n2) + ((- a3y) * n3)).y;

    const double a1z = n1.z >= 0 ? 1 : (-1);
    const double a2z = n2.z >= 0 ? 1 : (-1);
    const double a3z = n3.z >= 0 ? 1 : (-1);

    max_z = (position + (a1z * n1) + (a2z * n2) + (a3z * n3)).z;
    min_z = (position + ((- a1z) * n1) + ((- a2z) * n2) + ((- a3z) * n3)).z;
}


/* Specific to (standard) boxes: returns true if the ray r hits the box
   The box is assumed to be standard (axes are n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1)) */
bool box::is_hit_by(const ray& r) const {
    
    const rt::vector& dir = r.get_direction();
    const rt::vector& inv_dir = r.get_inv_dir();
    const rt::vector& abs_inv_dir = r.get_abs_inv_dir();

    // See measure_distance

    const rt::vector& u = r.get_origin();
    const double pmux = position.x - u.x;
    const double pmuy = position.y - u.y;
    const double pmuz = position.z - u.z;
    
    if (abs(pmux) <= l1 && abs(pmuy) <= l2 && abs(pmuz) <= l3) {
        // u is inside the box
        return true;
    }

    if (dir.x != 0) {
        // Determination of t1
        const double t1 = pmux * inv_dir.x - l1 * abs_inv_dir.x;
        // Check that t1 gives a point inside the face
        if (abs(pmuy - t1 * dir.y) <= l2 && abs(pmuz - t1 * dir.z) <= l3) {
            return (t1 >= 0);
        }
    }

    if (dir.y != 0) {
        const double t2 = pmuy * inv_dir.y - l2 * abs_inv_dir.y;
        if (abs(pmux - t2 * dir.x) <= l1 && abs(pmuz - t2 * dir.z) <= l3) {
            return (t2 >= 0);
        }
    }

    if (dir.z != 0) {
        const double t3 = pmuz * inv_dir.z - l3 * abs_inv_dir.z;
        return (t3 >= 0 && abs(pmux - t3 * dir.x) <= l1 && abs(pmuy - t3 * dir.y) <= l2);
    }

    return false;
}