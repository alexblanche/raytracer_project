#include "scene/objects/triangle.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"

#include <optional>


/* Constructors */

triangle::triangle() {}

// Constructor from three points without vertex normals
triangle::triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2, 
    const size_t material_index, const std::optional<texture_info>& info)

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

// Constructor from three points with vertex normals
triangle::triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2,
    const rt::vector& vn0, const rt::vector& vn1, const rt::vector& vn2,
    const size_t material_index, const std::optional<texture_info>& info)

    : polygon(p0, material_index, info), vn0(vn0.unit()), vn1(vn1.unit()), vn2(vn2.unit()) {
    
    v1 = p1 - p0;
    v2 = p2 - p0;
    const rt::vector n = (v1 ^ v2);
    normal = n.unit();
    d = - (normal | p0);
}

/* Returns the barycenter of the triangle */
rt::vector triangle::get_barycenter() const {
    return (position + (v1 + v2) / 3);
}

/* Intersection determination */

std::optional<real> triangle::measure_distance(const ray& r) const {
    const rt::vector u = r.get_origin();
    const rt::vector dir = r.get_direction();

    // Intersection between the ray and the triangle plane
    const real pdt = (normal | dir); // ax + by + cz
    const real upln = (normal | u) + d; // aX + bY + cZ + d

    // printf("u = (%lf, %lf, %lf), dir = (%lf, %lf, %lf), pdt = %lf, upln = %lf\n",
    //     u.x, u.y, u.z, dir.x, dir.y, dir.z, pdt, upln);
    
    // If -upln/pdt > 0, it is our solution t, otherwise the plane is either parallel (pdt == 0) or "behind" the plane (-upln/pdt < 0)
    if (pdt * upln >= 0.0f) {
        return std::nullopt;
    }

    const real t = - upln / pdt;

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
    const real detxy = v1.x * v2.y - v1.y * v2.x;

    // printf("c = (%lf, %lf, %lf), detxy = %lf, abs(detxy) = %lf, cond = %d\n", c.x, c.y, c.z, detxy, abs(detxy), abs(detxy) > 0.00000001);

    if (abs(detxy) > 0.00000001f) {
        const real l1 = (c.x * v2.y - c.y * v2.x) / detxy;
        if (l1 >= 0.0f && l1 <= 1.0f) {
            const real l2 = (v1.x * c.y - v1.y * c.x) / detxy;
            if (l2 >= 0.0f && l1 + l2 <= 1.0f) {
                return t;
            }
        }
    }
    else {
        // The vectors v1, v2 are colinear when projected on the plane z = 0
        // Another attempt with rows x, z
        const real detxz = v1.x * v2.z - v1.z * v2.x;
        if (abs(detxz) > 0.00000001f) {
            const real l1xz = (c.x * v2.z - c.z * v2.x) / detxz;
            if (l1xz >= 0.0f && l1xz <= 1.0f) {
                const real l2xz = (v1.x * c.z - v1.z * c.x) / detxz;
                if (l2xz >= 0.0f && l1xz + l2xz <= 1.0f) {
                    return t;
                }
            }
        }
        else {
            // The vectors v1, v2 are colinear when projected on the planes y = 0 and z = 0
            // (e.g. the triangle lies in the plane x = constant)
            // Last attempt with rows y, z
            const real detyz = v1.y * v2.z - v1.z * v2.y;
            if (abs(detyz) > 0.00000001f) {
                const real l1yz = (c.y * v2.z - c.z * v2.y) / detyz;
                if (l1yz >= 0.0f && l1yz <= 1.0f) {
                    const real l2yz = (v1.y * c.z - v1.z * c.y) / detyz;
                    if (l2yz >= 0.0f && l1yz + l2yz <= 1.0f) {
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
    const real detxy = v1.x * v2.y - v1.y * v2.x;
    if (abs(detxy) > 0.00000001) {
        const real l1 = (c.x * v2.y - c.y * v2.x) / detxy;
        const real l2 = (v1.x * c.y - v1.y * c.x) / detxy;
        return barycentric_info(l1, l2);
    }
    else {
        const real detxz = v1.x * v2.z - v1.z * v2.x;
        if (abs(detxz) > 0.00000001) {
            const real l1 = (c.x * v2.z - c.z * v2.x) / detxz;
            const real l2 = (v1.x * c.z - v1.z * c.x) / detxz;
            return barycentric_info(l1, l2);
        }
        else {
            const real detyz = v1.y * v2.z - v1.z * v2.y;
            const real l1 = (c.y * v2.z - c.z * v2.y) / detyz;
            const real l2 = (v1.y * c.z - v1.z * c.y) / detyz;
            return barycentric_info(l1, l2);
        }
    }
}

rt::vector triangle::get_interpolated_normal(const barycentric_info& bary) const {
    return (((1 - bary.l1 - bary.l2) * vn0) + (bary.l1 * vn1) + (bary.l2 * vn2));
}

hit triangle::compute_intersection(ray& r, const real t) const {
    
    const rt::vector p = r.get_origin() + t * r.get_direction();

    // Computation of the interpolated normal vector
    const barycentric_info bary = get_barycentric(p);

    const object* pt_obj = this;
    ray* pt_ray = &r;
    
    return hit(pt_ray, p, get_interpolated_normal(bary), pt_obj);
}


/* Minimum and maximum coordinates */
min_max_coord triangle::get_min_max_coord() const {

    const rt::vector p1 = position + v1;
    const rt::vector p2 = position + v2;

    const real min_x = std::min(position.x, std::min(p1.x, p2.x));
    const real max_x = std::max(position.x, std::max(p1.x, p2.x));

    const real min_y = std::min(position.y, std::min(p1.y, p2.y));
    const real max_y = std::max(position.y, std::max(p1.y, p2.y));
    
    const real min_z = std::min(position.z, std::min(p1.z, p2.z));
    const real max_z = std::max(position.z, std::max(p1.z, p2.z));

    return min_max_coord(min_x, max_x, min_y, max_y, min_z, max_z);
}

/* Prints the triangle */
void triangle::print() const {
    printf("p0 = (%lf, %lf, %lf), ", position.x, position.y, position.z);
    rt::vector p1 = position + v1;
    printf("p1 = (%lf, %lf, %lf), ", p1.x, p1.y, p1.z);
    rt::vector p2 = position + v2;
    printf("p2 = (%lf, %lf, %lf)\n", p2.x, p2.y, p2.z);
}