#include "scene/objects/triangle.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"

#include <optional>

/* Constructors */

triangle::triangle() {}

#define DEFAULT 0
#define CASE_XZ 1
#define CASE_YZ 2
#define CASE_ERROR 3

std::pair<real, int> set_up_det(const rt::vector& v1, const rt::vector& v2) {

    // printf("c = (%lf, %lf, %lf), detxy = %lf, abs(detxy) = %lf, cond = %d\n", c.x, c.y, c.z, detxy, abs(detxy), abs(detxy) > 0.00000001);
    
    const real detxy = v1.x * v2.y - v1.y * v2.x;
    if (abs(detxy) > 0.00000001f)
        return std::pair(detxy, DEFAULT);

    // The vectors v1, v2 are colinear when projected on the plane z = 0
    // Another attempt with rows x, z
    const real detxz = v1.x * v2.z - v1.z * v2.x;
    if (abs(detxz) > 0.00000001f)
        return std::pair(detxz, CASE_XZ);
    
    // The vectors v1, v2 are colinear when projected on the planes y = 0 and z = 0
    // (e.g. the triangle lies in the plane x = constant)
    // Last attempt with rows y, z
    const real detyz = v1.y * v2.z - v1.z * v2.y;
    if (abs(detyz) > 0.00000001f)
        return std::pair(detyz, CASE_YZ);

    return std::pair(0, CASE_ERROR);
    
}

// Constructor from three points without vertex normals
triangle::triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2, 
    const unsigned int material_index, const unsigned int texture_info_index)

    : object(p0, material_index, texture_info_index) {

    v1 = p1 - p0;
    v2 = p2 - p0;
    const rt::vector n = (v1 ^ v2);
    normal = n.unit();
    vn0 = normal;
    // vn1 = normal;
    // vn2 = normal;
    vn1mvn0 = rt::vector();
    vn2mvn0 = rt::vector();
    d = - (normal | p0);

    std::pair<real, int> p = set_up_det(v1, v2);
    det = p.first;
    case_det = p.second;
}

// Constructor from three points with vertex normals
triangle::triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2,
    const rt::vector& vn0init, const rt::vector& vn1, const rt::vector& vn2,
    const unsigned int material_index, const unsigned int texture_info_index)

    : object(p0, material_index, texture_info_index), vn0(vn0init.unit())
    //, vn1(vn1.unit()), vn2(vn2.unit())
    {
    
    v1 = p1 - p0;
    v2 = p2 - p0;
    const rt::vector n = (v1 ^ v2);
    normal = n.unit();
    d = - (normal | p0);

    vn1mvn0 = (vn1.unit()) - vn0;
    vn2mvn0 = (vn2.unit()) - vn0;

    std::pair<real, int> p = set_up_det(v1, v2);
    det = p.first;
    case_det = p.second;
}

// Constructor from three points with normal mapping enabled
triangle::triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2,
    const unsigned int material_index, const unsigned int texture_info_index, const bool normal_mapping,
    texture_info& info)

    : object(p0, material_index, texture_info_index) {
    
    v1 = p1 - p0;
    v2 = p2 - p0;
    const rt::vector n = (v1 ^ v2);
    normal = n.unit();
    vn0 = normal;
    // vn1 = normal;
    // vn2 = normal;
    vn1mvn0 = rt::vector();
    vn2mvn0 = rt::vector();

    d = - (normal | p0);

    std::pair<real, int> p = set_up_det(v1, v2);
    det = p.first;
    case_det = p.second;

    if (normal_mapping) {
        
        /*
        Computation of tangent space
        v1 = x1 * t + y1 * b
        v2 = x2 * t + y2 * b

        In matrix form:
        (v1.x v1.y v1.z)   (x1 y1)(t.x t.y t.z)
        (v2.x v2.y v2.z) = (x2 y2)(b.x b.y b.z)

        So,
        (x1 y1)-1 (v1.x v1.y v1.z)   (t.x t.y t.z)
        (x2 y2)   (v2.x v2.y v2.z) = (b.x b.y b.z)

        (x1 y1)-1                             (y2  -y1)
        (x2 y2)   = (1 / (x1 * y2 - x2 * y1)) (-x2  x1)
        */
    
        const std::vector<real>& uvc = info.uv_coordinates;
        // uvc = (u0, v0, u1, v1, u2, v2)
        const real x1 = uvc[2] - uvc[0];
        const real x2 = uvc[4] - uvc[0];
        const real y1 = uvc[3] - uvc[1];
        const real y2 = uvc[5] - uvc[1];
        const real r = 1.0f / (x1 * y2 - x2 * y1);
        const rt::vector t = r * ( y2 * v1 + -y1 * v2);
        const rt::vector b = r * (-x2 * v1 +  x1 * v2);
        info.set_tangent_space(t.unit(), b.unit());
    }
}

// Constructor from three points with vertex normals and normal mapping enabled
triangle::triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2,
    const rt::vector& vn0init, const rt::vector& vn1, const rt::vector& vn2,
    const unsigned int material_index, const unsigned int texture_info_index, const bool normal_mapping,
    texture_info& info)

    : object(p0, material_index, texture_info_index), vn0(vn0init.unit())
    //, vn1(vn1.unit()), vn2(vn2.unit())
    {
    
    v1 = p1 - p0;
    v2 = p2 - p0;
    const rt::vector n = (v1 ^ v2);
    normal = n.unit();
    d = - (normal | p0);

    vn1mvn0 = (vn1.unit()) - vn0;
    vn2mvn0 = (vn2.unit()) - vn0;

    std::pair<real, int> p = set_up_det(v1, v2);
    det = p.first;
    case_det = p.second;

    if (normal_mapping) {
        
        /*
        Computation of tangent space
        v1 = x1 * t + y1 * b
        v2 = x2 * t + y2 * b

        In matrix form:
        (v1.x v1.y v1.z)   (x1 y1)(t.x t.y t.z)
        (v2.x v2.y v2.z) = (x2 y2)(b.x b.y b.z)

        So,
        (x1 y1)-1 (v1.x v1.y v1.z)   (t.x t.y t.z)
        (x2 y2)   (v2.x v2.y v2.z) = (b.x b.y b.z)

        (x1 y1)-1                             (y2  -y1)
        (x2 y2)   = (1 / (x1 * y2 - x2 * y1)) (-x2  x1)
        */
    
        const std::vector<real>& uvc = info.uv_coordinates;
        // uvc = (u0, v0, u1, v1, u2, v2)
        const real x1 = uvc[2] - uvc[0];
        const real x2 = uvc[4] - uvc[0];
        const real y1 = uvc[3] - uvc[1];
        const real y2 = uvc[5] - uvc[1];
        const real r = 1.0f / (x1 * y2 - x2 * y1);
        const rt::vector t = r * ( y2 * v1 + -y1 * v2);
        const rt::vector b = r * (-x2 * v1 +  x1 * v2);
        info.set_tangent_space(t.unit(), b.unit());
    }
}

/* Returns the barycenter of the triangle */
inline rt::vector triangle::get_barycenter() const {
    return (position + (v1 + v2) / 3);
}

/* Intersection determination */

#if 0
std::optional<real> triangle::measure_distance_orig(const ray& r) const {
    const rt::vector& u = r.get_origin();
    const rt::vector& dir = r.get_direction();

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
            return (l2 >= 0.0f && l1 + l2 <= 1.0f) ?
                std::optional<real>(t) : std::nullopt;
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
                return (l2xz >= 0.0f && l1xz + l2xz <= 1.0f) ?
                    std::optional<real>(t) : std::nullopt;
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
                    return (l2yz >= 0.0f && l1yz + l2yz <= 1.0f) ?
                        std::optional<real>(t) : std::nullopt;
                }
            }
        }
    }

    return std::nullopt;
}
#endif

std::optional<real> triangle::measure_distance(const ray& r) const {
    const rt::vector& u = r.get_origin();
    const rt::vector& dir = r.get_direction();

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

    //const rt::vector c = u + (t * dir) - position;
    const rt::vector c = fma(dir, t, u) - position;

    // printf("c = (%lf, %lf, %lf), detxy = %lf, abs(detxy) = %lf, cond = %d\n", c.x, c.y, c.z, detxy, abs(detxy), abs(detxy) > 0.00000001);

    real l1;
    switch (case_det)
    {
    case DEFAULT: l1 = (c.x * v2.y - c.y * v2.x) / det;
        break;
    case CASE_XZ: l1 = (c.x * v2.z - c.z * v2.x) / det;
        break;
    case CASE_YZ: l1 = (c.y * v2.z - c.z * v2.y) / det;
        break;
    default: l1 = 0.0f;
        break;
    }

    if (l1 < 0.0f || l1 > 1.0f)
        return std::nullopt;

    real l2;
    switch (case_det)
    {
    case DEFAULT: l2 = (v1.x * c.y - v1.y * c.x) / det;
        break;
    case CASE_XZ: l2 = (v1.x * c.z - v1.z * c.x) / det;
        break;
    case CASE_YZ: l2 = (v1.y * c.z - v1.z * c.y) / det;
        break;
    default: l2 = 0.0f;
        break;
    }

    return (l2 >= 0.0f && l1 + l2 <= 1.0f) ?
        std::optional<real>(t) : std::nullopt;
}

/* Returns the barycentric info (l1, l2):
   p = position + l1 * v1 + l2 * v2
   (0 <= l1, l2 <= 1)
*/
#if 0
barycentric_info triangle::get_barycentric_orig(const rt::vector& p) const {

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
#endif

barycentric_info triangle::get_barycentric(const rt::vector& p) const {

    const rt::vector c = p - position;

    real l1, l2;
    switch (case_det)
    {
    case DEFAULT:
        l1 = (c.x * v2.y - c.y * v2.x) / det;
        l2 = (v1.x * c.y - v1.y * c.x) / det;
        break;

    case CASE_XZ:
        l1 = (c.x * v2.z - c.z * v2.x) / det;
        l2 = (v1.x * c.z - v1.z * c.x) / det;
        break;

    case CASE_YZ:
        l1 = (c.y * v2.z - c.z * v2.y) / det;
        l2 = (v1.y * c.z - v1.z * c.y) / det;
        break;

    default:
        l1 = 0.0f;
        l2 = 0.0f;
        break;
    }

    return barycentric_info(l1, l2);
}

inline rt::vector triangle::get_interpolated_normal(const barycentric_info& bary) const {
    // return (((1 - bary.l1 - bary.l2) * vn0) + (bary.l1 * vn1) + (bary.l2 * vn2));
    //return vn0 + (bary.l1 * vn1mvn0) + (bary.l2 * vn2mvn0);
    return fma(vn2mvn0, bary.l2, fma(vn1mvn0, bary.l1, vn0));
}

hit triangle::compute_intersection(ray& r, const real t) const {
    
    //const rt::vector p = r.get_origin() + t * r.get_direction();
    const rt::vector p = fma(r.get_direction(), t, r.get_origin());
    const object* pt_obj = this;
    ray* pt_ray = &r;
    
    if constexpr (SMOOTH_SHADING) {
    
        // Computation of the interpolated normal vector
        const barycentric_info bary = get_barycentric(p);
        // inward uses the face normal to avoid artefacts at the edge of the mesh
        const bool inward = (r.get_direction() | normal) <= 0.0f;

        return hit(pt_ray, p, get_interpolated_normal(bary), pt_obj, inward);
    }
    else { // Flat shading
        
        return hit(pt_ray, p, normal, pt_obj);
    }
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


/* Normal map vector computation at render time
    Local normal may be the normal of the triangle (for flat shading) or the smoothed normal, and in this case the tangent space should be reorthonormalized */
rt::vector triangle::compute_normal_from_map(const rt::vector& tangent_space_normal, const rt::vector& local_normal, const texture_info& info) const {

    if constexpr (SMOOTH_SHADING) {
        const rt::vector& t = info.tangent;
        // Recompute the tangent space with Gram-Schmidt's method
        /*
        const rt::vector t2 = (t - ((t | local_normal) * local_normal)).unit();
        const rt::vector b2 = t2 ^ local_normal;

        //return tangent_space_normal.x * t2 + tangent_space_normal.y * b2 + tangent_space_normal.z * local_normal;
        return matprod(
            t2,             tangent_space_normal.x,
            b2,             tangent_space_normal.y,
            local_normal,   tangent_space_normal.z
        );
        */
        const rt::vector t2_non_unit = t - ((t | local_normal) * local_normal);
        const rt::vector b2_non_unit = t2_non_unit ^ local_normal;
        const real norm_t2 = t2_non_unit.norm();

        //return tangent_space_normal.x * t2 + tangent_space_normal.y * b2 + tangent_space_normal.z * local_normal;
        return matprod(
            t2_non_unit,  tangent_space_normal.x / norm_t2,
            b2_non_unit,  tangent_space_normal.y / norm_t2,
            local_normal, tangent_space_normal.z
        );
    }
    else {
        // Flat shading
        const rt::vector& t = info.tangent;
        const rt::vector& b = info.bitangent;

        //return tangent_space_normal.x * t + tangent_space_normal.y * b + tangent_space_normal.z * local_normal;
        return matprod(
            t,            tangent_space_normal.x,
            b,            tangent_space_normal.y,
            local_normal, tangent_space_normal.z
        );
    }
}


inline rt::vector triangle::sample(randomgen& rg) const {
    
    /* Incorrect version: does not sample uniformly (will do for now) */
    const real u = rg.random_real(1.0f);
    const real v = rg.random_real(1.0f - u);

    return position + (u * v1) + (v * v2);

}

inline rt::vector triangle::sample_visible(randomgen& rg, const rt::vector& /*pt*/) const {
    return sample(rg);
}