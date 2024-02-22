#include "headers/box.hpp"

#include "../../light/headers/vector.hpp"
#include "../../light/headers/hit.hpp"
#include "../material/headers/material.hpp"

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


/* Accessors */

double box::get_l1() const {
    return l1;
}

double box::get_l2() const {
    return l2;
}

double box::get_l3() const {
    return l3;
}


/* Intersection determination */

double box::measure_distance(const ray& r) const {
    /* For the face orthogonal to n1, we search for a t1 that satisfies:
       ((pos + l1.n1) - (u + t.dir) | n1) = 0 (if (dir | n1) < 0, and otherwise pos - l1.n1 instead of pos + l1.n1)
       where u is the origin of the ray, and dir its direction,
       So t1 = ((pos + l1.n1 - u) | n1) / (dir | n1)
       We then check whether |((pos + l1.n1 - (u + t1.dir)) | n2)| <= l2 and |(((pos + l1.n1 - (u + t1.dir)) | n3)| <= l3,
       to make sure the intersection point lies on the face.

       Same with the other two directions, we obtain t2, t3.
       If (dir | n) = 0 for n among n1, n2, n3, then the ray does not intersect the plane and the associated t is infinity.
    */

    const rt::vector& dir = r.get_direction();
    const rt::vector& u = r.get_origin();

    /** This whole function to be optimized (it will then become incomprehensible)
     * 
     * const vector pmu = position - u;
     * t1 = ((c1 - u) | n1) / pdt1
     *      = ((position - (l1 * pdt1 / abs(pdt1)) * n1 - u) | n1) / pdt1;
     *      = ((pmu | n1) - l1 * pdt1 / abs(pdt1)) / pdt1;
     *      = ((pmu | n1) / pdt1 - l1 / abs(pdt1));
     * 
     * (p | n2)
     *      = ((c1 - u - (t1*dir)) | n2)
     *      = ((position - (...)*n1 - u - t1*dir) | n2)
     *      = (pmu | n2) - t1 * pdt2
     * **/

    const rt::vector pmu = position - u;
    const double pmun1 = (pmu | n1);
    const double pmun2 = (pmu | n2);
    const double pmun3 = (pmu | n3);

    // TODO: delete this condition and adapt the code to work inside the box
    if (abs(pmun1) <= l1 && abs(pmun2) <= l2 && abs(pmun3) <= l3) {
        // u is inside the box
        return 0;
    }

    const double pdt1 = (dir | n1);
    const double pdt2 = (dir | n2);
    const double pdt3 = (dir | n3);

    double t1 = infinity;
    const double abspdt1 = abs(pdt1);

    if (abspdt1 > 0.0000001) {
        // Determination of t1
        t1 = pmun1 / pdt1 - l1 / abspdt1;

        // Check that t1 gives a point inside the face
        const double checkpdt2 = pmun2 - t1 * pdt2;
        if (abs(checkpdt2) <= l2) {
            const double checkpdt3 = pmun3 - t1 * pdt3;
            if (abs(checkpdt3) > l3) {
                t1 = infinity;
            }
        }
        else {
            t1 = infinity;
        }

        if (t1 < 0) { return infinity; }
    }

    double t2 = infinity;
    const double abspdt2 = abs(pdt2);

    if (abspdt2 > 0.0000001) {
        t2 = pmun2 / pdt2 - l2 / abspdt2;
    
        const double checkpdt1 = pmun1 - t2 * pdt1;
        if (abs(checkpdt1) <= l1) {
            const double checkpdt3 = pmun3 - t2 * pdt3;
            if (abs(checkpdt3) > l3) {
                t2 = infinity;
            }
        }
        else {
            t2 = infinity;
        }

        if (t2 < 0) { return infinity; }
    }

    double t3 = infinity;
    const double abspdt3 = abs(pdt3);

    if (abspdt3 > 0.0000001) {
        t3 = pmun3 / pdt3 - l3 / abspdt3;
        
        const double checkpdt1 = pmun1 - t3 * pdt1;
        if (abs(checkpdt1) <= l1) {
            const double checkpdt2 = pmun2 - t3 * pdt2;
            if (abs(checkpdt2) > l2) {
                t3 = infinity;
            }
        }
        else {
            t3 = infinity;
        }

        if (t3 < 0) { return infinity; }
    }

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
    */

    return std::min(t1, std::min(t2, t3));
}
        
hit box::compute_intersection(const ray& r, const double t) const {
    // Intersection point
    const rt::vector p = r.get_origin() + t * r.get_direction();

    // Re-computing the face of intersection
    // (not great, but the alternative is to return a hit object (with distance t) for every intersection check)

    const rt::vector v = p - position;
    const double pdt1 = (v | n1);
    if (abs(pdt1 - l1) < 0.0000001) {
        return hit(r, p, n1, get_index());
    }
    else if (abs(pdt1 + l1) < 0.0000001) {
        return hit(r, p, (-1)*n1, get_index());
    }
    else {
        const double pdt2 = (v | n2);
        if (abs(pdt2 - l2) < 0.0000001) {
            return hit(r, p, n2, get_index());
        }
        else if (abs(pdt2 + l2) < 0.0000001) {
            return hit(r, p, (-1)*n2, get_index());
        }
        else {
            const double pdt3 = (v | n3);
            if (abs(pdt3 - l3) < 0.0000001) {
                return hit(r, p, n3, get_index());
            }
            else {
                return hit(r, p, (-1)*n3, get_index());
            } 
        }   
    }   
}

bool box::does_hit(const ray& r) const {

    // Fall-back option
    //return (measure_distance(r) != infinity);
    
    const rt::vector& dir = r.get_direction();

    // See measure_distance

    const rt::vector pmu = position - r.get_origin();
    const double pmun1 = (pmu | n1);
    const double pmun2 = (pmu | n2);
    const double pmun3 = (pmu | n3);
    
    if (abs(pmun1) <= l1 && abs(pmun2) <= l2 && abs(pmun3) <= l3) {
        // u is inside the box
        return true;
    }

    const double pdt1 = (dir | n1);
    const double pdt2 = (dir | n2);
    const double pdt3 = (dir | n3);

    const double abspdt1 = abs(pdt1);

    if (abspdt1 > 0.0000001) {
        // Determination of t1
        const double t1 = pmun1 / pdt1 - l1 / abspdt1;
        if (t1 < 0) { return false; }

        // Check that t1 gives a point inside the face
        const double checkpdt2 = pmun2 - t1 * pdt2;
        if (abs(checkpdt2) <= l2) {
            const double checkpdt3 = pmun3 - t1 * pdt3;
            if (abs(checkpdt3) <= l3) { return true; }
        }
    }

    const double abspdt2 = abs(pdt2);

    if (abspdt2 > 0.0000001) {
        const double t2 = pmun2 / pdt2 - l2 / abspdt2;
        if (t2 < 0) { return false; }
    
        const double checkpdt1 = pmun1 - t2 * pdt1;
        if (abs(checkpdt1) <= l1) {
            const double checkpdt3 = pmun3 - t2 * pdt3;
            if (abs(checkpdt3) <= l3) { return true; }
        }
    }

    const double abspdt3 = abs(pdt3);

    if (abspdt3 > 0.0000001) {
        const double t3 = pmun3 / pdt3 - l3 / abspdt3;
        if (t3 < 0) { return false; }
        
        const double checkpdt1 = pmun1 - t3 * pdt1;
        if (abs(checkpdt1) <= l1) {
            const double checkpdt2 = pmun2 - t3 * pdt2;
            if (abs(checkpdt2) <= l2) { return true; }
        }   
    }

    return false;
}