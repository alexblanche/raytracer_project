#include "headers/triangle.hpp"

#include "../../light/headers/vector.hpp"
#include "../../light/headers/hit.hpp"
#include "../material/headers/material.hpp"

#include<limits>
numeric_limits<double> realtr;
const double infinity = realtr.infinity();


/* Constructors */

triangle::triangle() : normal(rt::vector()), v1(rt::vector()), v2(rt::vector()), d(0) {}
        
// Constructor from three points
triangle::triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2, 
    const material& material)

    : object(p0, material) {

    v1 = p1 - p0;
    v2 = p2 - p0;
    const rt::vector n = (v1 ^ v2);
    normal = n.unit();
    vn0 = normal;
    vn1 = normal;
    vn2 = normal;
    d = - (normal | p0);
}

// Constructor from three points with vertex normals
triangle::triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2,
    const rt::vector& vn0, const rt::vector& vn1, const rt::vector& vn2,
    const material& material)

    : object(p0, material), vn0(vn0.unit()), vn1(vn1.unit()), vn2(vn2.unit()) {
    
    v1 = p1 - p0;
    v2 = p2 - p0;
    const rt::vector n = (v1 ^ v2);
    normal = n.unit();
    d = - (normal | p0);
}

/* Accessors */

rt::vector triangle::get_normal() const {
    return normal;
}

rt::vector triangle::get_v1() const {
    return v1;
}

rt::vector triangle::get_v2() const {
    return v2;
}


/* Intersection determination */

double triangle::measure_distance(const ray& r) const {
    const rt::vector& u = r.get_origin();
    const rt::vector& dir = r.get_direction();

    // Intersection between the ray and the triangle plane
    const double pdt = (normal | dir); // ax + by + cz
    const double upln = (normal | u) + d; // aX + bY + cZ + d
    
    // If -upln/pdt > 0, it is our solution t, otherwise the plane is either parallel (pdt == 0) or "behind" the plane (-upln/pdt < 0)
    if (pdt * upln >= 0) {
        return infinity;
    }

    const double t = - upln / pdt;

    // Check if the point of intersection lies inside the triangle
    /* The system we try to solve is:
       l1 * v1 + l2 * v2 = c (= pt - pos), so 3 equations (on x, y, z) and two variables.
       We calculate the solution to the rows x and y (it necessarily solves row z)
       
       According to Cramer's rule, the solution to
       ax + by = e
       cx + dy = f
       is x = (ed-bf)/(ad-bc) and y = (af-ec)/(ad-bc).
       
       In our case, when det = v1x * v2y - v1y * v2x,
       l1 = (cx * v2y - cy * v2x) / det and l2 = (v1x * cy - v1y * cx) / det */

    const rt::vector c = u + (t * dir) - position;
    const double detxy = v1.x * v2.y - v1.y * v2.x;

    if (abs(detxy) > 0.00001) {
        const double l1 = (c.x * v2.y - c.y * v2.x) / detxy;
        if (l1 >= 0 && l1 <= 1) {
            const double l2 = (v1.x * c.y - v1.y * c.x) / detxy;
            if (l2 >= 0 && l1 + l2 <= 1) {
                return t;
            }
            else {
                return infinity;
            }
        }
        else {
            return infinity;
        }
    }
    else {
        // The vectors v1, v2 are colinear when projected on the plane z = 0
        // Another attempt with rows x, z
        const double detxz = v1.x * v2.z - v1.z * v2.x;
        if (abs(detxz) > 0.00001) {
            const double l1xz = (c.x * v2.z - c.z * v2.x) / detxz;
            if (l1xz >= 0 && l1xz <= 1) {
                const double l2xz = (v1.x * c.z - v1.z * c.x) / detxz;
                if (l2xz >= 0 && l1xz + l2xz <= 1) {
                    return t;
                }
                else {
                    return infinity;
                }
            }
            else {
                return infinity;
            }
        }
        else {
            // The vectors v1, v2 are colinear when projected on the planes y = 0 and z = 0
            // (e.g. the triangle lies in the plane x = constant)
            // Last attempt with rows y, z
            const double detyz = v1.y * v2.z - v1.z * v2.y;
            if (abs(detyz) > 0.00001) {
                const double l1yz = (c.y * v2.z - c.z * v2.y) / detyz;
                if (l1yz >= 0 && l1yz <= 1) {
                    const double l2yz = (v1.y * c.z - v1.z * c.y) / detyz;
                    if (l2yz >= 0 && l1yz + l2yz <= 1) {
                        return t;
                    }
                    else {
                        return infinity;
                    }
                }
                else {
                    return infinity;
                }
            }
            else {
                return infinity;
            }
        }
    }
}

/* Writes the barycentric coordinates in variables l1, l2:
   p = position + l1 * v1 + l2 * v2
   (0 <= l1, l2 <= 1)
*/
void triangle::get_barycentric(const rt::vector& p, double& l1, double& l2) const {

    const rt::vector c = p - position;
    const double detxy = v1.x * v2.y - v1.y * v2.x;
    if (abs(detxy) > 0.00001) {
        l1 = (c.x * v2.y - c.y * v2.x) / detxy;
        l2 = (v1.x * c.y - v1.y * c.x) / detxy;
    }
    else {
        const double detxz = v1.x * v2.z - v1.z * v2.x;
        if (abs(detxz) > 0.00001) {
            l1 = (c.x * v2.z - c.z * v2.x) / detxz;
            l2 = (v1.x * c.z - v1.z * c.x) / detxz;
        }
        else {
            const double detyz = v1.y * v2.z - v1.z * v2.y;
            l1 = (c.y * v2.z - c.z * v2.y) / detyz;
            l2 = (v1.y * c.z - v1.z * c.y) / detyz;
        }
    }
}

rt::vector triangle::get_interpolated_normal(const double& l1, const double& l2) const {
    return (((1 - l1 - l2) * vn0) + (l1 * vn1) + (l2 * vn2));
}

hit triangle::compute_intersection(const ray& r, const double t) const {
    const rt::vector p = r.get_origin() + t * r.get_direction();

    // Computation of the interpolated normal vector
    double l1, l2;
    get_barycentric(p, l1, l2);
    // Also used to get the texture info (to be implemented here later)

    return hit(r, p, get_interpolated_normal(l1, l2), get_index());
}