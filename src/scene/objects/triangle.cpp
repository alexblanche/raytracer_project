#include "scene/objects/triangle.hpp"

#include "auxiliary/utils.hpp"

using enum det_case;

static std::pair<real, det_case> set_up_det(const rt::vector& v1, const rt::vector& v2) {

    // det(XY) = 0 => v1, v2 are collinear when projected onto the plane z = 0
    // det(XZ) = 0 => v1, v2 are collinear when projected onto the planes y = 0 and z = 0
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
    d = - (normal | p0);

    const auto [ det_, case_det_ ] = set_up_det(v1, v2);
    det      = det_;
    case_det = case_det_;
}

// Constructor from three points with vertex normals
triangle::triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2,
    const rt::vector& vn0init, const rt::vector& vn1, const rt::vector& vn2,
    const unsigned int material_index, const unsigned int texture_info_index)

    : triangle(p0, p1, p2, material_index, texture_info_index) {

    vn0     = vn0init.unit();
    vn1mvn0 = (vn1.unit()) - vn0;
    vn2mvn0 = (vn2.unit()) - vn0;
}

// Constructor from three points with normal mapping enabled
triangle::triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2,
    const unsigned int material_index, const unsigned int texture_info_index,
    texture_info& info)

    : triangle(p0, p1, p2, material_index, texture_info_index) {

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

    const auto& [ u_0, v_0, u_1, v_1, u_2, v_2, _, _ ] = info.uv_coordinates;
    const real x1 = u_1 - u_0;
    const real x2 = u_2 - u_0;
    const real y1 = v_1 - v_0;
    const real y2 = v_2 - v_0;
    const real r = 1.0_r / (x1 * y2 - x2 * y1);
    const rt::vector t = r * ( y2 * v1 + -y1 * v2);
    const rt::vector b = r * (-x2 * v1 +  x1 * v2);
    info.set_tangent_space(t.unit(), b.unit());
    
}

// Constructor from three points with vertex normals and normal mapping enabled
triangle::triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2,
    const rt::vector& vn0init, const rt::vector& vn1, const rt::vector& vn2,
    const unsigned int material_index, const unsigned int texture_info_index,
    texture_info& info)

    : triangle(p0, p1, p2, material_index, texture_info_index, info) {
    
    vn0     = vn0init.unit();
    vn1mvn0 = (vn1.unit()) - vn0;
    vn2mvn0 = (vn2.unit()) - vn0;
}

/* Returns the barycenter of the triangle */
inline rt::vector triangle::get_barycenter() const {
    return (position + (v1 + v2) / 3);
}

/* Intersection determination */

real triangle::measure_distance(const ray& r) const {

    const auto& [ u, dir, _ ] = r;

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

    const rt::vector c = r.extend(t) - position;

    const real l1 = compute_det_2d(c, v2, case_det) / det;
    if (not is_between_zero_and_one(l1))
        return infinity;

    const real l2 = compute_det_2d(v1, c, case_det) / det;
    return (is_positive(l2) && (l1 + l2) <= 1.0_r) ? t : infinity;
}

/* Returns the barycentric info (l1, l2):
   p = position + l1 * v1 + l2 * v2
   (0 <= l1, l2 <= 1)
*/
barycentric_info triangle::get_barycentric(const rt::vector& p) const {

    const rt::vector c = p - position;

    const real l1 = compute_det_2d(c, v2, case_det) / det;
    const real l2 = compute_det_2d(v1, c, case_det) / det;

    return barycentric_info(l1, l2, object_type::Triangle);
}

inline rt::vector triangle::get_interpolated_normal(const barycentric_info& bary) const {
    
    const auto [ l1, l2 ] = bary.l;
    return fma(vn2mvn0, l2, fma(vn1mvn0, l1, vn0));
}

hit triangle::compute_intersection(const ray& r, const real t) const {
    
    const rt::vector p = r.extend(t);
    const object* pt_obj = this;
    
    if constexpr (SHADING == shading::SmoothShading) {
    
        // Computation of the interpolated normal vector
        const barycentric_info bary = get_barycentric(p);

        // ray_orientation uses the face normal to avoid artefacts at the edge of the mesh
        using enum orientation_type;
        const orientation_type ray_orientation = is_negative(r.direction | normal) ? Inward : Outward;

        return hit(p, get_interpolated_normal(bary), pt_obj, ray_orientation);
    }
    else { // Flat shading
        
        return hit(r.direction, p, normal, pt_obj);
    }
}


/* Minimum and maximum coordinates */
min_max_coord triangle::get_min_max_coord() const {

    const rt::vector p1 = position + v1;
    const rt::vector p2 = position + v2;

    // const auto& [ min12_x, max12_x ] = (p1.x < p2.x) ? std::pair{ p1.x, p2.x } : std::pair{ p2.x, p1.x };
    // const auto& [ min12_y, max12_y ] = (p1.y < p2.y) ? std::pair{ p1.y, p2.y } : std::pair{ p2.y, p1.y };
    // const auto& [ min12_z, max12_z ] = (p1.z < p2.z) ? std::pair{ p1.z, p2.z } : std::pair{ p2.z, p1.z };

    const auto& [ min12, max12 ] = rt::min_max(p1, p2);

    const rt::vector min = rt::min(position, min12);
    const rt::vector max = rt::max(position, max12);

    return build_min_max_coord(min, max);
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

inline rt::vector triangle::sample(const randomgen& rg) const {
    
    // Samples uniformly inside the triangle
    
    return sample_triangle(rg, position, v1, v2);
}

inline rt::vector triangle::sample_visible(const randomgen& rg, const rt::vector&) const {
    return sample(rg);
}

void triangle::print() const {
    printf("triangle: ");
    printf("p0 = ");
    position.print();
    const rt::vector p1 = position + v1;
    printf(", p1 = ");
    p1.print();
    const rt::vector p2 = position + v2;
    printf(", p2 = ");
    p2.print();
    printf("\n");
}