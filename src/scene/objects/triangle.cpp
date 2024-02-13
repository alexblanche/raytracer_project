#include "headers/triangle.hpp"

#include "../../light/headers/vector.hpp"
#include "../../light/headers/hit.hpp"
#include "../material/headers/material.hpp"

#include<limits>
numeric_limits<double> realtr;
const double infinity = realtr.infinity();


/* Constructors */

triangle::triangle() : p(plane()), v1(rt::vector()), v2(rt::vector()) {}
        
// Constructor from the stored the parameters
triangle::triangle(const plane& p, const unsigned int index,
    const rt::vector& position, const rt::vector& v1, const rt::vector& v2, const material& material)

    : object(position, index, material), p(p), v1(v1), v2(v2) {}
        
// Constructor from three points
triangle::triangle(const rt::vector& p1, const rt::vector& p2, const rt::vector& p3, 
    const unsigned int index, const material& material)

    : object(p1, index, material) {

    v1 = p2 - p1;
    v2 = p3 - p1;
    const rt::vector n = (v1 ^ v2);
    p = plane(n.x, n.y, n.z, p1, index, material);
}

/* Accessors */

rt::vector triangle::get_normal() const {
    return p.get_normal();
}


/* Intersection determination */

double triangle::measure_distance(const ray& r) const {
    // Intersection of the ray and the plane p
    const double t = p.measure_distance(r);
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

    if (det * det < 0.00001) {
        return infinity;
    }
    else {
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
                // Useless: it always does
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
}
        
hit triangle::compute_intersection(const ray& r, const double t) const {
    return p.compute_intersection(r, t);
}

/* Returns a vector (only the first two coordinates matter) with the barycentric coordinates (l1, l2):
   p = position + l1 * v1 + l2 * v2
*/
rt::vector triangle::get_barycentric(const rt::vector& p) const {
    const rt::vector c = p - position;
    const double det = v1.x * v2.y - v1.y * v2.x;
    const double l1 = (c.x * v2.y - c.y * v2.x) / det;
    const double l2 = (v1.x * c.y - v1.y * c.x) / det;
    return rt::vector(l1, l2, 0);
}