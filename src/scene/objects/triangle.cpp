#include "scene/objects/triangle.hpp"

#include "scene/material/material.hpp"
#include "auxiliary/utils.hpp"

#include <optional>

using enum det_case;

static inline real compute_det_2d(const rt::vector& v1, const rt::vector& v2, det_case det_case) {

    switch (det_case) {
        case XY:    return v1.x * v2.y - v1.y * v2.x;
        case XZ:    return v1.x * v2.z - v1.z * v2.x;
        case YZ:    return v1.y * v2.z - v1.z * v2.y;
        case Error: return 0.0_r;
    }
}

std::pair<real, det_case> set_up_det(const rt::vector& v1, const rt::vector& v2) {

    // det(XY) = 0 => v1, v2 are colinear when projected onto the plane z = 0
    // det(XZ) = 0 => v1, v2 are colinear when projected onto the planes y = 0 and z = 0
    //    (e.g. the triangle lies in the plane x = constant)
    
    for (det_case det_case : { XY, XZ, YZ }) {
        const real det = compute_det_2d(v1, v2, det_case);
        if (is_not_zero(det))
            return { det, det_case };
    }

    return { 0.0_r, Error };
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
    // vn1mvn0 = rt::vector();
    // vn2mvn0 = rt::vector();
    d = - (normal | p0);

    const auto [ d, case_d ] = set_up_det(v1, v2);
    det      = d;
    case_det = case_d;
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

    const auto [ d, case_d ] = set_up_det(v1, v2);
    det      = d;//std::max(d, 1.0e-10_r);
    case_det = case_d;
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
    // vn1mvn0 = rt::vector();
    // vn2mvn0 = rt::vector();

    d = - (normal | p0);

    std::pair<real, det_case> p = set_up_det(v1, v2);
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
        const real r = 1.0_r / (x1 * y2 - x2 * y1);
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

    const auto [ d, case_d ] = set_up_det(v1, v2);
    det      = d;
    case_det = case_d;

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
        const real r = 1.0_r / (x1 * y2 - x2 * y1);
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
    const rt::vector& u   = r.origin;
    const rt::vector& dir = r.direction;

    // Intersection between the ray and the triangle plane
    const real pdt = (normal | dir); // ax + by + cz
    const real upln = (normal | u) + d; // aX + bY + cZ + d

    // printf("u = (%lf, %lf, %lf), dir = (%lf, %lf, %lf), pdt = %lf, upln = %lf\n",
    //     u.x, u.y, u.z, dir.x, dir.y, dir.z, pdt, upln);
    
    // If -upln/pdt > 0, it is our solution t, otherwise the plane is either parallel (pdt == 0) or "behind" the plane (-upln/pdt < 0)
    if (pdt * upln >= 0.0_r) {
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

    // printf("c = (%lf, %lf, %lf), detxy = %lf, abs(detxy) = %lf, cond = %d\n", c.x, c.y, c.z, detxy, std::abs(detxy), std::abs(detxy) > 0.00000001);

    if (std::abs(detxy) > 0.00000001f) {
        const real l1 = (c.x * v2.y - c.y * v2.x) / detxy;
        if (l1 >= 0.0_r && l1 <= 1.0_r) {
            const real l2 = (v1.x * c.y - v1.y * c.x) / detxy;
            return (l2 >= 0.0_r && l1 + l2 <= 1.0_r) ?
                std::optional<real>(t) : std::nullopt;
        }
    }
    else {
        // The vectors v1, v2 are colinear when projected on the plane z = 0
        // Another attempt with rows x, z
        const real detxz = v1.x * v2.z - v1.z * v2.x;
        if (std::abs(detxz) > 0.00000001f) {
            const real l1xz = (c.x * v2.z - c.z * v2.x) / detxz;
            if (l1xz >= 0.0_r && l1xz <= 1.0_r) {
                const real l2xz = (v1.x * c.z - v1.z * c.x) / detxz;
                return (l2xz >= 0.0_r && l1xz + l2xz <= 1.0_r) ?
                    std::optional<real>(t) : std::nullopt;
            }
        }
        else {
            // The vectors v1, v2 are colinear when projected on the planes y = 0 and z = 0
            // (e.g. the triangle lies in the plane x = constant)
            // Last attempt with rows y, z
            const real detyz = v1.y * v2.z - v1.z * v2.y;
            if (std::abs(detyz) > 0.00000001f) {
                const real l1yz = (c.y * v2.z - c.z * v2.y) / detyz;
                if (l1yz >= 0.0_r && l1yz <= 1.0_r) {
                    const real l2yz = (v1.y * c.z - v1.z * c.y) / detyz;
                    return (l2yz >= 0.0_r && l1yz + l2yz <= 1.0_r) ?
                        std::optional<real>(t) : std::nullopt;
                }
            }
        }
    }

    return std::nullopt;
}
#endif

real triangle::measure_distance(const ray& r) const {

    const rt::vector& u   = r.origin;
    const rt::vector& dir = r.direction;

    // Intersection between the ray and the triangle plane
    const real pdt  = (normal | dir);   // ax + by + cz
    const real upln = (normal | u) + d; // aX + bY + cZ + d

    // If -upln/pdt > 0, it is our solution t, otherwise the plane is either parallel (pdt == 0) or "behind" the plane (-upln/pdt < 0)
    //if (is_positive(pdt * upln))
    if (std::signbit(pdt) == std::signbit(upln))
        return infinity;

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

    const rt::vector c = fma(dir, t, u) - position;

    const real l1 = compute_det_2d(c, v2, case_det) / det;
    if (not is_between_zero_and_one(l1))
        return infinity;

    const real l2 = compute_det_2d(v1, c, case_det) / det;
    return (is_positive(l2) && is_between_zero_and_one(l1 + l2)) ? t : infinity;
}

/* Returns the barycentric info (l1, l2):
   p = position + l1 * v1 + l2 * v2
   (0 <= l1, l2 <= 1)
*/
#if 0
barycentric_info triangle::get_barycentric_orig(const rt::vector& p) const {

    const rt::vector c = p - position;
    const real detxy = v1.x * v2.y - v1.y * v2.x;
    if (std::abs(detxy) > 0.00000001) {
        const real l1 = (c.x * v2.y - c.y * v2.x) / detxy;
        const real l2 = (v1.x * c.y - v1.y * c.x) / detxy;
        return barycentric_info(l1, l2);
    }
    else {
        const real detxz = v1.x * v2.z - v1.z * v2.x;
        if (std::abs(detxz) > 0.00000001) {
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

    const real l1 = compute_det_2d(c, v2, case_det) / det;
    const real l2 = compute_det_2d(v1, c, case_det) / det;

    return barycentric_info(l1, l2, object_type::Triangle);
}

inline rt::vector triangle::get_interpolated_normal(const barycentric_info& bary) const {
    
    return fma(vn2mvn0, bary.l2, fma(vn1mvn0, bary.l1, vn0));
}

hit triangle::compute_intersection(const ray& r, const real t) const {
    
    const rt::vector p = fma(r.direction, t, r.origin);
    const object* pt_obj = this;
    const ray* pt_ray = &r;
    
    if constexpr (SHADING == shading::SmoothShading) {
    
        // Computation of the interpolated normal vector
        const barycentric_info bary = get_barycentric(p);

        // ray_orientation uses the face normal to avoid artefacts at the edge of the mesh
        using enum orientation_type;
        const orientation_type ray_orientation = is_negative(r.direction | normal) ? Inward : Outward;

        return hit(pt_ray, p, get_interpolated_normal(bary), pt_obj, ray_orientation);
    }
    else { // Flat shading
        
        return hit(pt_ray, p, normal, pt_obj);
    }
}


/* Minimum and maximum coordinates */
min_max_coord triangle::get_min_max_coord() const {

    const rt::vector p1 = position + v1;
    const rt::vector p2 = position + v2;

    return {
        .min_x = std::min(position.x, std::min(p1.x, p2.x)),
        .max_x = std::max(position.x, std::max(p1.x, p2.x)),
        .min_y = std::min(position.y, std::min(p1.y, p2.y)),
        .max_y = std::max(position.y, std::max(p1.y, p2.y)),
        .min_z = std::min(position.z, std::min(p1.z, p2.z)),
        .max_z = std::max(position.z, std::max(p1.z, p2.z))
    };
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

    if constexpr (SHADING == shading::SmoothShading) {
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
        return matprod(t, b, local_normal, tangent_space_normal);
    }
}


inline rt::vector triangle::sample(randomgen& rg) const {
    
    /* Incorrect version: does not sample uniformly (will do for now) */
    const real u = rg.random_ratio();
    const real v = rg.random_real(1.0_r - u);

    return position + (u * v1) + (v * v2);

}

inline rt::vector triangle::sample_visible(randomgen& rg, const rt::vector&) const {
    return sample(rg);
}