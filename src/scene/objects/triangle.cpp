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


/* Intersection determination */

double triangle::measure_distance(const ray& r) const {
    // Intersection between the ray and the triangle plane
    const double pdt = (normal | r.get_direction()); // ax + by + cz
    const double upln = (normal | r.get_origin()) + d; // aX + bY + cZ + d
    
    // If -upln/pdt > 0, it is our solution t, otherwise the plane is either parallel (pdt == 0) or "behind" the plane (-upln/pdt < 0)
    if (pdt * upln >= 0) {
        return infinity;
    }

    const double t = - upln / pdt;
    const rt::vector pt = r.get_origin() + (t * r.get_direction());

    // Check if the point of intersection lies inside the triangle
    /* The system we try to solve is:
       l1 * v1 + l2 * v2 = c (= pt - pos), so 3 equations (on x, y, z) and two variables.
       We calculate the solution to the rows x and y, and then check if it solves the row z.
       
       According to Cramer's rule, the solution to
       ax + by = e
       cx + dy = f
       is x = (ed-bf)/(ad-bc) and y = (af-ec)/(ad-bc).
       
       In our case, when det = v1x * v2y - v1y * v2x,
       l1 = (cx * v2y - cy * v2x) / det and l2 = (v1x * cy - v1y * cx) / det */

    const rt::vector c = pt - position;
    const double det = v1.x * v2.y - v1.y * v2.x;

    if (det * det > 0.00001) {
        const double l1 = (c.x * v2.y - c.y * v2.x) / det;
        if (l1 >= 0 && l1 <= 1) {
            const double l2 = (v1.x * c.y - v1.y * c.x) / det;
            if (l2 >= 0 && l1 + l2 <= 1) {
                // Checking if row z is satisfied by our solution
                /*
                const double z = l1 * v1.z + l2 * v2.z - c.z;
                if (z*z < 0.001) {
                    return t;
                }
                else {
                    return infinity;
                }
                */
                // Useless: it is always satisfied
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
        // Another attempt with rows x, z (if it fails, it also fails for rows y, z)
        const double detxz = v1.x * v2.z - v1.z * v2.x;
        if (detxz * detxz > 0.00001) {
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
            return infinity;
        }
    }
}

/* Returns a vector (only the first two coordinates matter) with the barycentric coordinates (l1, l2):
   p = position + l1 * v1 + l2 * v2
   (0 <= l1, l2 <= 1)
*/
rt::vector triangle::get_barycentric(const rt::vector& p) const {
    const rt::vector c = p - position;
    const double det = v1.x * v2.y - v1.y * v2.x;
    if (det * det > 0.00001) {
        const double l1 = (c.x * v2.y - c.y * v2.x) / det;
        const double l2 = (v1.x * c.y - v1.y * c.x) / det;
        return rt::vector(l1, l2, 0);
    }
    else {
        const double detxz = v1.x * v2.z - v1.z * v2.x;
        const double l1 = (c.x * v2.z - c.z * v2.x) / detxz;
        const double l2 = (v1.x * c.z - v1.z * c.x) / detxz;
        return rt::vector(l1, l2, 0);
    }
}

rt::vector triangle::get_interpolated_normal(const double& l1, const double& l2) const {
    return (((1 - l1 - l2) * vn0) + (l1 * vn1) + (l2 * vn2));
}

hit triangle::compute_intersection(const ray& r, const double t) const {
    const rt::vector p = r.get_origin() + t * r.get_direction();

    // Computation of the interpolated normal vector
    const rt::vector bc = get_barycentric(p);
    // Also used to get the texture info (to be implemented here later)

    return hit(r, p, get_interpolated_normal(bc.x, bc.y), get_index());
}