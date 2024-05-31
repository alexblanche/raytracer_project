#include "scene/objects/triangle.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"

#include <optional>


/* Constructors */

triangle::triangle() : normal(rt::vector()), v1(rt::vector()), v2(rt::vector()), d(0) {}
        
// Constructor from three points
triangle::triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2, 
    const unsigned int material_index)

    : polygon(p0, material_index) {

    v1 = p1 - p0;
    v2 = p2 - p0;
    const rt::vector n = (v1 ^ v2);
    normal = n.unit();
    vn0 = normal;
    vn1 = normal;
    vn2 = normal;
    d = - (normal | p0);

    // printf("TRIANGLE (from 3 points):\n");
    // printf("p0 = (%lf, %lf, %lf), v1 = (%lf, %lf, %lf), v2 = (%lf, %lf, %lf)\n",
    //     p0.x, p0.y, p0.z, v1.x, v1.y, v1.z, v2.x, v2.y, v2.z);
    // printf("vn0 = (%lf, %lf, %lf), vn1 = (%lf, %lf, %lf), vn2 = (%lf, %lf, %lf)\n",
    //     vn0.x, vn0.y, vn0.z, vn1.x, vn1.y, vn1.z, vn2.x, vn2.y, vn2.z);
    // printf("normal = (%lf, %lf, %lf)\n", normal.x, normal.y, normal.z);
    // printf("d = %lf\n", d);
}

// Constructor from three points with vertex normals
triangle::triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2,
    const rt::vector& vn0, const rt::vector& vn1, const rt::vector& vn2,
    const unsigned int material_index)

    : polygon(p0, material_index), vn0(vn0.unit()), vn1(vn1.unit()), vn2(vn2.unit()) {
    
    v1 = p1 - p0;
    v2 = p2 - p0;
    const rt::vector n = (v1 ^ v2);
    normal = n.unit();
    d = - (normal | p0);
}

// Constructors for textured triangles
triangle::triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2, 
    const unsigned int material_index, const texture_info& info)

    : polygon(p0, material_index, info) {

    v1 = p1 - p0;
    v2 = p2 - p0;
    const rt::vector n = (v1 ^ v2);
    normal = n.unit();
    vn0 = normal;
    vn1 = normal;
    vn2 = normal;
    d = - (normal | p0);
}

triangle::triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2,
    const rt::vector& vn0, const rt::vector& vn1, const rt::vector& vn2,
    const unsigned int material_index, const texture_info& info)

    : polygon(p0, material_index, info), vn0(vn0.unit()), vn1(vn1.unit()), vn2(vn2.unit()) {
    
    v1 = p1 - p0;
    v2 = p2 - p0;
    const rt::vector n = (v1 ^ v2);
    normal = n.unit();
    d = - (normal | p0);
}



/* Intersection determination */

std::optional<double> triangle::measure_distance(const ray& r) const {
    const rt::vector u = r.get_origin();
    const rt::vector dir = r.get_direction();

    // Intersection between the ray and the triangle plane
    const double pdt = (normal | dir); // ax + by + cz
    const double upln = (normal | u) + d; // aX + bY + cZ + d

    // printf("u = (%lf, %lf, %lf), dir = (%lf, %lf, %lf), pdt = %lf, upln = %lf\n",
    //     u.x, u.y, u.z, dir.x, dir.y, dir.z, pdt, upln);
    
    // If -upln/pdt > 0, it is our solution t, otherwise the plane is either parallel (pdt == 0) or "behind" the plane (-upln/pdt < 0)
    if (pdt * upln >= 0) {
        return std::nullopt;
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

    // printf("c = (%lf, %lf, %lf), detxy = %lf, abs(detxy) = %lf, cond = %d\n", c.x, c.y, c.z, detxy, abs(detxy), abs(detxy) > 0.00000001);

    if (abs(detxy) > 0.00000001) {
        const double l1 = (c.x * v2.y - c.y * v2.x) / detxy;
        if (l1 >= 0 && l1 <= 1) {
            const double l2 = (v1.x * c.y - v1.y * c.x) / detxy;
            if (l2 >= 0 && l1 + l2 <= 1) {
                return t;
            }
        }
    }
    else {
        // The vectors v1, v2 are colinear when projected on the plane z = 0
        // Another attempt with rows x, z
        const double detxz = v1.x * v2.z - v1.z * v2.x;
        if (abs(detxz) > 0.00000001) {
            const double l1xz = (c.x * v2.z - c.z * v2.x) / detxz;
            if (l1xz >= 0 && l1xz <= 1) {
                const double l2xz = (v1.x * c.z - v1.z * c.x) / detxz;
                if (l2xz >= 0 && l1xz + l2xz <= 1) {
                    return t;
                }
            }
        }
        else {
            // The vectors v1, v2 are colinear when projected on the planes y = 0 and z = 0
            // (e.g. the triangle lies in the plane x = constant)
            // Last attempt with rows y, z
            const double detyz = v1.y * v2.z - v1.z * v2.y;
            if (abs(detyz) > 0.00000001) {
                const double l1yz = (c.y * v2.z - c.z * v2.y) / detyz;
                if (l1yz >= 0 && l1yz <= 1) {
                    const double l2yz = (v1.y * c.z - v1.z * c.y) / detyz;
                    if (l2yz >= 0 && l1yz + l2yz <= 1) {
                        return t;
                    }
                }
            }
        }
    }

    return std::nullopt;
}

/* Writes the barycentric coordinates in variables l1, l2:
   p = position + l1 * v1 + l2 * v2
   (0 <= l1, l2 <= 1)
*/
barycentric_info triangle::get_barycentric(const rt::vector& p) const {

    const rt::vector c = p - position;
    const double detxy = v1.x * v2.y - v1.y * v2.x;
    if (abs(detxy) > 0.00000001) {
        const double l1 = (c.x * v2.y - c.y * v2.x) / detxy;
        const double l2 = (v1.x * c.y - v1.y * c.x) / detxy;
        return barycentric_info(l1, l2);
    }
    else {
        const double detxz = v1.x * v2.z - v1.z * v2.x;
        if (abs(detxz) > 0.00000001) {
            const double l1 = (c.x * v2.z - c.z * v2.x) / detxz;
            const double l2 = (v1.x * c.z - v1.z * c.x) / detxz;
            return barycentric_info(l1, l2);
        }
        else {
            const double detyz = v1.y * v2.z - v1.z * v2.y;
            const double l1 = (c.y * v2.z - c.z * v2.y) / detyz;
            const double l2 = (v1.y * c.z - v1.z * c.y) / detyz;
            return barycentric_info(l1, l2);
        }
    }
}

rt::vector triangle::get_interpolated_normal(const barycentric_info& bary) const {
    return (((1 - bary.l1 - bary.l2) * vn0) + (bary.l1 * vn1) + (bary.l2 * vn2));
}

hit triangle::compute_intersection(ray& r, const double& t) const {
    
    const rt::vector p = r.get_origin() + t * r.get_direction();

    // Computation of the interpolated normal vector
    const barycentric_info bary = get_barycentric(p);

    const object* pt_obj = this;
    ray* pt_ray = &r;
    
    return hit(pt_ray, p, get_interpolated_normal(bary), pt_obj);
}


/* Minimum and maximum coordinates */
void triangle::min_max_coord(double& min_x, double& max_x,
    double& min_y, double& max_y, double& min_z, double& max_z) const {

    const rt::vector p1 = position + v1;
    const rt::vector p2 = position + v2;

    min_x = std::min(position.x, std::min(p1.x, p2.x));
    max_x = std::max(position.x, std::max(p1.x, p2.x));

    min_y = std::min(position.y, std::min(p1.y, p2.y));
    max_y = std::max(position.y, std::max(p1.y, p2.y));
    
    min_z = std::min(position.z, std::min(p1.z, p2.z));
    max_z = std::max(position.z, std::max(p1.z, p2.z));
}