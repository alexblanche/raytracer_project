#include "scene/objects/quad.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"

#include<limits>
numeric_limits<double> realq;
const double infinity = realq.infinity();


/* Constructors */

quad::quad() : normal(rt::vector()), v1(rt::vector()), v2(rt::vector()), v3(rt::vector()), d(0) {}
        
// Constructor from four points
// We do not check whether the four points are coplanar
quad::quad(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2, const rt::vector& p3, 
    const material& material, const unsigned int index)

    : object(p0, material, index) {

    v1 = p1 - p0;
    v2 = p2 - p0;
    v3 = p3 - p0;
    const rt::vector n = (v1 ^ v2);
    normal = n.unit();
    vn0 = normal;
    vn1 = normal;
    vn2 = normal;
    vn3 = normal;
    d = - (normal | p0);
}

// Constructor from four points with vertex normals
quad::quad(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2, const rt::vector& p3,
    const rt::vector& vn0, const rt::vector& vn1, const rt::vector& vn2, const rt::vector& vn3,
    const material& material, const unsigned int index)

    : object(p0, material, index), vn0(vn0.unit()), vn1(vn1.unit()), vn2(vn2.unit()), vn3(vn3.unit()) {
    
    v1 = p1 - p0;
    v2 = p2 - p0;
    v3 = p3 - p0;
    const rt::vector n = (v1 ^ v2);
    normal = n.unit();
    d = - (normal | p0);
}



/* Intersection determination */

double quad::measure_distance(const ray& r) const {
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

    /* See triangle intersection.
       Here we check if the points lies in the triangle generated by v1, v2
       (by solving the system for x, y, and if they are colinear when projected on the plane z = 0, for x, z),
       if it does not, we check if it lies in the triangle generated by v2, v3 in the same way. */

	const rt::vector c = u + (t * dir) - position;
    const double det12xy = v1.x * v2.y - v1.y * v2.x;

    // Checking if the point lies in the triangle generated by v1, v2
    if (abs(det12xy) > 0.00001) {
        const double detv2cxy = c.x * v2.y - c.y * v2.x;
        const double l1 = detv2cxy / det12xy;
        const double l2 = (v1.x * c.y - v1.y * c.x) / det12xy;
        if (l1 >= 0 && l2 >= 0 && l1 <= 1 && l1 + l2 <= 1) {
            return t;
		}
        else {
            // Checking if the point lies in the triangle generated by v2, v3

            const double det23xy = v3.x * v2.y - v3.y * v2.x;
            // det23 is necessary != 0
            const double l1 = detv2cxy / det23xy;
            if (l1 >= 0 && l1 <= 1) {
                const double l2 = (v3.x * c.y - v3.y * c.x) / det23xy;
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
    }
    else {
        // The vectors v1, v2 are colinear when projected on the plane z = 0
        const double det12xz = v1.x * v2.z - v1.z * v2.x;
        if (abs(det12xz) > 0.00001) {
            const double detv2cxz = c.x * v2.z - c.z * v2.x;
            const double l1xz = detv2cxz / det12xz;
            const double l2xz = (v1.x * c.z - v1.z * c.x) / det12xz;
            if (l1xz >= 0 && l2xz >= 0 && l1xz <= 1 && l1xz + l2xz <= 1) {
                return t;
			}
            else {
                // The vectors v2, v3 are colinear when projected on the plane z = 0
                const double det23xz = v3.x * v2.z - v3.z * v2.x;
                // det23xz is necessary != 0
                const double l1xz = detv2cxz / det23xz;
                if (l1xz >= 0 && l1xz <= 1) {
                    const double l2xz = (v3.x * c.z - v3.z * c.x) / det23xz;
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
		}
        else {
            // The vectors v1, v2 are colinear when projected on the planes y = 0 and z = 0
            const double det12yz = v1.y * v2.z - v1.z * v2.y;
            if (abs(det12yz) > 0.00001) {
                const double detv2cyz = c.y * v2.z - c.z * v2.y;
                const double l1yz = detv2cyz / det12yz;
                const double l2yz = (v1.y * c.z - v1.z * c.y) / det12yz;
                if (l1yz >= 0 && l2yz >= 0 && l1yz <= 1 && l1yz + l2yz <= 1) {
                    return t;
                }
                else {
                    // The vectors v2, v3 are colinear when projected on the planes y = 0 and z = 0
                    const double det23yz = v3.y * v2.z - v3.z * v2.y;
                    // det23yz is necessary != 0
                    const double l1yz = detv2cyz / det23yz;
                    if (l1yz >= 0 && l1yz <= 1) {
                        const double l2yz = (v3.y * c.z - v3.z * c.y) / det23yz;
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
            }
            else {
                return infinity;
            }
        }
    }
}

/* Writes the barycentric coordinates in variables l1, l2, and returns the boolean lower_triangle:
   (0 <= l1, l2 <= 1)
   p = position + l1 * v1 + l2 * v2 if lower_triangle == true,
   or
   p = position + l1 * v3 + l2 * v2 otherwise
*/
bool quad::get_barycentric(const rt::vector& p, double& l1, double& l2) const {

    const rt::vector c = p - position;
    const double det12xy = v1.x * v2.y - v1.y * v2.x;
    if (det12xy != 0) {
        const double detv2cxy = c.x * v2.y - c.y * v2.x;
        l1 = detv2cxy / det12xy;
        l2 = (v1.x * c.y - v1.y * c.x) / det12xy;
        if (l1 >= 0 && l2 >= 0 && l1 <= 1 && l1 + l2 <= 1) {
            return true;
        }
        else {
            const double det23 = v3.x * v2.y - v3.y * v2.x;
            l1 = detv2cxy / det23;
            l2 = (v3.x * c.y - v3.y * c.x) / det23;
            return false;
        }
    }
    else {
        const double det12xz = v1.x * v2.z - v1.z * v2.x;
        if (det12xz != 0) {
            const double detv2cxz = c.x * v2.z - c.z * v2.x;
            l1 = detv2cxz / det12xz;
            l2 = (v1.x * c.z - v1.z * c.x) / det12xz;
            if (l1 >= 0 && l2 >= 0 && l1 <= 1 && l1 + l2 <= 1) {
                return true;
            }
            else {
                const double det23xz = v3.x * v2.z - v3.z * v2.x;
                l1 = detv2cxz / det23xz;
                l2 = (v3.x * c.z - v3.z * c.x) / det23xz;
                return false;
            }
        }
        else {
            const double det12yz = v1.y * v2.z - v1.z * v2.y;
            const double detv2cyz = c.y * v2.z - c.z * v2.y;
            l1 = detv2cyz / det12yz;
            l2 = (v1.y * c.z - v1.z * c.y) / det12yz;
            if (l1 >= 0 && l2 >= 0 && l1 <= 1 && l1 + l2 <= 1) {
                return true;
            }
            else {
                const double det23yz = v3.y * v2.z - v3.z * v2.y;
                l1 = detv2cyz / det23yz;
                l2 = (v3.y * c.z - v3.z * c.y) / det23yz;
                return false;
            }
        }
    }
}

rt::vector quad::get_interpolated_normal(const double& l1, const double& l2, const bool triangle) const {
    if (triangle) {
        return (((1 - l1 - l2) * vn0) + (l1 * vn1) + (l2 * vn2));
    }
    else {
        return (((1 - l1 - l2) * vn0) + (l1 * vn3) + (l2 * vn2));
    }
    
}

hit quad::compute_intersection(const ray& r, const double t) const {
    const rt::vector p = r.get_origin() + t * r.get_direction();

    // Computation of the interpolated normal vector
    double l1, l2;
    bool triangle = get_barycentric(p, l1, l2);
    // Also used to get the texture info (to be implemented here later)

    return hit(r, p, get_interpolated_normal(l1, l2, triangle), get_index());
}

/* Minimum and maximum coordinates */
void quad::min_max_coord(double& min_x, double& max_x,
    double& min_y, double& max_y, double& min_z, double& max_z) const {

    const rt::vector p1 = position + v1;
    const rt::vector p2 = position + v2;
    const rt::vector p3 = position + v3;

    min_x = std::min(position.x, std::min(p1.x, std::min(p2.x, p3.x)));
    max_x = std::max(position.x, std::max(p1.x, std::max(p2.x, p3.x)));

    min_y = std::min(position.y, std::min(p1.y, std::min(p2.y, p3.y)));
    max_y = std::max(position.y, std::max(p1.y, std::max(p2.y, p3.y)));
    
    min_z = std::min(position.z, std::min(p1.z, std::min(p2.z, p3.z)));
    max_z = std::max(position.z, std::max(p1.z, std::max(p2.z, p3.z)));
}