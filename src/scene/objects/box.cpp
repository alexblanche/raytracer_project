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


/* Intersection determination */

double box::measure_distance(const ray& r) const {
    /* For the face orthogonal to n1, we search for a t1 that satisfies:
       ((pos + l1.n1) - (u + t.dir) | n1) = 0 (if (dir | n1) < 0, and otherwise pos - l1.n1 instead of pos + l1.n1)
       where u is the origin of the ray, and dir its direction,
       So t1 = ((pos + l1.n1 - u) | n1) / (dir | n1)
       We then check whether |((pos + l1.n1 - (u + t1.dir)) | n2)| <= l2 and |(((pos + l1.n1 - (u + t1.dir)) | n3)| <= l3,
       to make sure the intersection point lies on the face.s

       Same with the other two directions, we obtain t2, t3.
       If (dir | n) = 0 for n among n1, n2, n3, then the ray does not intersect the plane and the associated t is infinity.
    */

    const rt::vector dir = r.get_direction();
    const rt::vector u = r.get_origin();
    double t1 = infinity;
    double t2 = infinity;
    double t3 = infinity;

    const double pdt1 = (dir | n1);
    const double pdt2 = (dir | n2);
    const double pdt3 = (dir | n3);

    if (pdt1 * pdt1 > 0.0000001) {
        // Determination of t1
        const rt::vector c1 = position - (l1 * pdt1 / abs(pdt1)) * n1;
        t1 = ((c1 - u) | n1) / pdt1;

        // Check that t1 gives a point inside the face
        const rt::vector p = u + (t1 * dir);
        const double checkpdt2 = ((c1 - p) | n2);
        if (checkpdt2 * checkpdt2 <= l2 * l2) {
            const double checkpdt3 = ((c1 - p) | n3);
            if (checkpdt3 * checkpdt3 > l3 * l3) {
                t1 = infinity;
            }
        }
        else {
            t1 = infinity;
        }

        if (t1 < 0) { return infinity; }
    }

    if (pdt2 * pdt2 > 0.0000001) {
        const rt::vector c2 = position - (l2 * pdt2/ abs(pdt2)) * n2;
        t2 = ((c2 - u) | n2) / pdt2;
    
        const rt::vector p = u + (t2 * dir);
        const double checkpdt1 = ((c2 - p) | n1);
        if (checkpdt1 * checkpdt1 <= l1 * l1) {
            const double checkpdt3 = ((c2 - p) | n3);
            if (checkpdt3 * checkpdt3 > l3 * l3) {
                t2 = infinity;
            }
        }
        else {
            t2 = infinity;
        }

        if (t2 < 0) { return infinity; }
    }

    if (pdt3 * pdt3 > 0.0000001) {
        const rt::vector c3 = position - (l3 * pdt3/ abs(pdt3)) * n3;
        t3 = ((c3 - u) | n3) / pdt3;
        
        const rt::vector p = u + (t3 * dir);
        const double checkpdt1 = ((c3 - p) | n1);
        if (checkpdt1 * checkpdt1 <= l1 * l1) {
            const double checkpdt2 = ((c3 - p) | n2);
            if (checkpdt2 * checkpdt2 > l2 * l2) {
                t3 = infinity;
            }
        }
        else {
            t3 = infinity;
        }

        if (t3 < 0) { return infinity; }
    }

    return std::min(t1, std::min(t2, t3));
}
        
hit box::compute_intersection(const ray& r, const double t) const {
    // Intersection point
    const rt::vector p = r.get_origin() + t * r.get_direction();

    // Re-computing the face of intersection
    // (not great, but the alternative is to return a hit object (with distance t) for every intersection check)

    const rt::vector v = p - position;
    const double pdt1 = (v | n1);
    if ((pdt1 - l1)*(pdt1 - l1) < 0.0000001) {
        return hit(r, p, n1, get_index());
    }
    else if ((pdt1 + l1)*(pdt1 + l1) < 0.0000001) {
        return hit(r, p, (-1)*n1, get_index());
    }
    else {
        const double pdt2 = (v | n2);
        if ((pdt2 - l2)*(pdt2 - l2) < 0.0000001) {
            return hit(r, p, n2, get_index());
        }
        else if ((pdt2 + l2)*(pdt2 + l2) < 0.0000001) {
            return hit(r, p, (-1)*n2, get_index());
        }
        else {
            const double pdt3 = (v | n3);
            if ((pdt3 - l3)*(pdt3 - l3) < 0.0000001) {
                return hit(r, p, n3, get_index());
            }
            else {
                return hit(r, p, (-1)*n3, get_index());
            } 
        }   
    }   
}