#include "scene/objects/triangle.hpp"

#include "auxiliary/utils.hpp"

using enum det_case;

static std::pair<real, det_case> set_up_det(const rt::vector& v1, const rt::vector& v2) {

    // det(XY) = 0 => v1, v2 are collinear when projected onto the plane z = 0
    // det(XZ) = 0 => v1, v2 are collinear when projected onto the planes y = 0 and z = 0
    //    (e.g. the triangle lies in the plane x = constant)
    
    for (det_case det_case : { XY, XZ, YZ }) {
        const real det = compute_det_2d(v1, v2, det_case);
        // Arbitrary bound: some determinants very close to 0 can break the computations
        if (std::abs(det) > DET_EPSILON)
            return { det, det_case };
    }

    return { 0.0_r, Error };
}

// Constructor from three points without vertex normals
triangle::triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2, 
    const unsigned int material_index, const unsigned int orientation_info_index)

    : object(p0, material_index, orientation_info_index) {

    v1 = p1 - p0;
    v2 = p2 - p0;
    const rt::vector n = (v1 ^ v2);
    normal = n.unit();
    vn0 = normal;
    d = - (normal | p0);

    const auto [ det_, case_det_ ] = set_up_det(v1, v2);
    det      = det_;
    case_det = case_det_;
}

// Constructor from three points with vertex normals
triangle::triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2,
    const rt::vector& vn0init, const rt::vector& vn1, const rt::vector& vn2,
    const unsigned int material_index, const unsigned int orientation_info_index)

    : triangle(p0, p1, p2, material_index, orientation_info_index) {

    vn0  = vn0init.unit();
    dvn1 = (vn1.unit()) - vn0;
    dvn2 = (vn2.unit()) - vn0;
}

//     Computation of tangent space
//     v1 = x1 * t + y1 * b
//     v2 = x2 * t + y2 * b

//     In matrix form:
//     (v1.x v1.y v1.z)   (x1 y1)(t.x t.y t.z)
//     (v2.x v2.y v2.z) = (x2 y2)(b.x b.y b.z)

//     So,
//     (x1 y1)-1 (v1.x v1.y v1.z)   (t.x t.y t.z)
//     (x2 y2)   (v2.x v2.y v2.z) = (b.x b.y b.z)

//     (x1 y1)-1                             (y2  -y1)
//     (x2 y2)   = (1 / (x1 * y2 - x2 * y1)) (-x2  x1)
//     */

triangle::orientation::orientation(const mapping::index_type index,
    const std::array<uvcoord, 3>& uvc, const rt::vector& v1, const rt::vector& v2)

    : mapping_info(index) {

    const auto& [ uv0, uv1, uv2 ] = uvc;
    uv[0] = uv0;
    uv[1] = uv1;
    uv[2] = uv2;

    const auto& [ u_0, v_0 ] = uv0;
    const auto& [ u_1, v_1 ] = uv1;
    const auto& [ u_2, v_2 ] = uv2;
    const real x1 = u_1 - u_0;
    const real x2 = u_2 - u_0;
    const real y1 = v_1 - v_0;
    const real y2 = v_2 - v_0;
    const real r = 1.0_r / (x1 * y2 - x2 * y1);

    tangent   = r * ( y2 * v1 + -y1 * v2);
    bitangent = r * (-x2 * v1 +  x1 * v2);
}

/* Returns the barycenter of the triangle */
inline rt::vector triangle::get_barycenter() const {
    return (position + (v1 + v2) / 3);
}

/* Intersection determination */

real triangle::measure_distance(const ray& r) const {
    // See Math-details.md

    const auto& [ u, dir, _ ] = r;

    const real pdt  = (normal | dir);
    const real upln = (normal | u) + d;

    if (std::signbit(pdt) == std::signbit(upln))
        return infinity;

    const real t = - upln / pdt;
    const rt::vector c = r.extend(t) - position;

    const real l1 = compute_det_2d(c, v2, case_det) / det;
    if (not is_between_zero_and_one(l1))
        return infinity;

    const real l2 = compute_det_2d(v1, c, case_det) / det;
    return (is_positive(l2) && (l1 + l2) <= 1.0_r) ? t : infinity;
}

stcoord triangle::compute_st(const rt::vector& p) const {
    const rt::vector c = p - position;
    return {
        .s = compute_det_2d(c, v2, case_det) / det,
        .t = compute_det_2d(v1, c, case_det) / det
    };
}

uvcoord triangle::compute_uv(const rt::vector& p, const mapping_info* orientation_info) const {

    const orientation& o = *static_cast<const orientation*>(orientation_info);

    /* ST-coordinates */
    const auto& [ s, t ] = compute_st(p);

    /* UV-coordinates */
    const auto& [ u0, v0 ] = o.uv[0];
    const auto& [ u1, v1 ] = o.uv[1];
    const auto& [ u2, v2 ] = o.uv[2];
    const real w = 1.0_r - s - t;

    return {
        .u = w * u0 + s * u1 + t * u2,
        .v = w * v0 + s * v1 + t * v2
    };
}

inline rt::vector triangle::compute_interpolated_normal(const stcoord& st) const {

    const auto& [ s, t ] = st;
    return fma(dvn2, t, fma(dvn1, s, vn0));
}

hit triangle::compute_intersection(const ray& r, const real t) const {
    
    const rt::vector p = r.extend(t);

    // ray_orientation uses the face normal (instead of the normal from the normal map)
    // to avoid artefacts at the edge of the mesh
    const ray_orientation_type ray_orientation = hit::compute_ray_orientation(r.direction, normal);
    
    if constexpr (SHADING == shading::SmoothShading) {
    
        // Computation of the interpolated normal vector
        const auto [ s, t ] = compute_st(p);
        const rt::vector interpolated_normal = fma(dvn2, t, fma(dvn1, s, vn0));

        return hit(p, interpolated_normal, this, ray_orientation, object_type::Triangle);
    }
    else { // Flat shading
        
        return hit(p, normal, this, ray_orientation, object_type::Triangle);
    }
}


/* Minimum and maximum coordinates */
min_max_coord triangle::get_min_max_coord() const {

    const rt::vector p1 = position + v1;
    const rt::vector p2 = position + v2;

    const auto& [ min12, max12 ] = rt::min_max(p1, p2);

    const min_max_vectors min_max = {
        .min = rt::min(position, min12),
        .max = rt::max(position, max12)
    };

    return build_min_max_coord(min_max);
}


/* Normal map vector computation at render time
    Local normal may be the normal of the triangle (for flat shading) or the smoothed normal, and in this case the tangent space should be reorthonormalized */
rt::vector triangle::compute_normal_from_map(const rt::vector& tangent_space_normal, const rt::vector& local_normal,
    const mapping_info* orientation_info) const {

    const orientation& o = *static_cast<const orientation*>(orientation_info);

    if constexpr (SHADING == shading::SmoothShading) {
        const rt::vector& t = o.tangent;
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
        const rt::vector& t = o.tangent;
        const rt::vector& b = o.bitangent;

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
    printf("Triangle: ");
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