#include "scene/objects/quad.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"

#include <optional>


/* Constructors */

quad::quad() {}
        
// Constructor from four points
// We do not check whether the four points are coplanar
quad::quad(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2, const rt::vector& p3, 
    const size_t material_index, const std::optional<texture_info>& info)

    : object(p0, material_index, info) {

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
    const size_t material_index, const std::optional<texture_info>& info)

    : object(p0, material_index, info), vn0(vn0.unit()), vn1(vn1.unit()), vn2(vn2.unit()), vn3(vn3.unit()) {
    
    v1 = p1 - p0;
    v2 = p2 - p0;
    v3 = p3 - p0;
    const rt::vector n = (v1 ^ v2);
    normal = n.unit();
    d = - (normal | p0);
}

// Constructor from four points with vertex normals and normal mapping enabled
quad::quad(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2, const rt::vector& p3,
    const rt::vector& vn0, const rt::vector& vn1, const rt::vector& vn2, const rt::vector& vn3,
    const size_t material_index, const std::optional<texture_info>& info, const bool normal_mapping)

    : object(p0, material_index, info), vn0(vn0.unit()), vn1(vn1.unit()), vn2(vn2.unit()), vn3(vn3.unit()) {
    
    v1 = p1 - p0;
    v2 = p2 - p0;
    v3 = p3 - p0;
    const rt::vector n = (v1 ^ v2);
    normal = n.unit();
    d = - (normal | p0);

    if (normal_mapping && info.has_value()) {
        
        /* Same as triangle */
    
        const std::vector<real>& uvc = texture_information.value().uv_coordinates;
        // uvc = (u0, v0, u1, v1, u2, v2)
        const real x1 = uvc[2] - uvc[0];
        const real x2 = uvc[4] - uvc[0];
        const real y1 = uvc[3] - uvc[1];
        const real y2 = uvc[5] - uvc[1];
        const real r = 1.0f / (x1 * y2 - x2 * y1);
        const rt::vector t = r * ( y2 * v1 + -y1 * v2);
        const rt::vector b = r * (-x2 * v1 +  x1 * v2);
        texture_information.value().set_tangent_space(t.unit(), b.unit());
    }
}

/* Returns the barycenter of the quad */
rt::vector quad::get_barycenter() const {
    return position + ((v1 + v2 + v3) / 4);
}

/* Intersection determination */

std::optional<real> quad::measure_distance(const ray& r) const {
    const rt::vector& u = r.get_origin();
    const rt::vector& dir = r.get_direction();
    
    // const rt::vector n12 = (v1 ^ v2).unit();
    // const rt::vector n23 = (v2 ^ v3).unit();
    // printf("(%lf,%lf,%lf) (%lf,%lf,%lf)\n", n12.x, n12.y, n12.z, n23.x, n23.y, n23.z);

    // Intersection between the ray and the triangle plane
    const real pdt = (normal | dir); // ax + by + cz
    const real upln = (normal | u) + d; // aX + bY + cZ + d
    
    // If -upln/pdt > 0, it is our solution t, otherwise the plane is either parallel (pdt == 0) or "behind" the plane (-upln/pdt < 0)
    if (pdt * upln >= 0.0f) {
        return std::nullopt;
    }

    const real t = - upln / pdt;

    /* See triangle intersection.
       Here we check if the points lies in the triangle generated by v1, v2
       (by solving the system for x, y, and if they are colinear when projected on the plane z = 0, for x, z),
       if it does not, we check if it lies in the triangle generated by v2, v3 in the same way. */

	const rt::vector c = u + (t * dir) - position;
    const real det12xy = v1.x * v2.y - v1.y * v2.x;

    // Checking if the point lies in the triangle generated by v1, v2
    if (abs(det12xy) > 0.00000001f) {
        const real detv2cxy = c.x * v2.y - c.y * v2.x;
        const real l1xy = detv2cxy / det12xy;
        const real l2xy = (v1.x * c.y - v1.y * c.x) / det12xy;
        if (l1xy >= 0.0f && l2xy >= 0.0f && l1xy <= 1.0f && l1xy + l2xy <= 1.0f) {
            return t;
		}
        else {
            // Checking if the point lies in the triangle generated by v2, v3

            const real det23xy = v3.x * v2.y - v3.y * v2.x;
            // det23xy is necessarily != 0
            const real l1axy = detv2cxy / det23xy;
            if (l1axy >= 0.0f && l1axy <= 1.0f) {
                const real l2axy = (v3.x * c.y - v3.y * c.x) / det23xy;
                if (l2axy >= 0.0f && l1axy + l2axy <= 1.0f) {
                    return t;
                }
            }
        }
    }
    else {

        // The vectors v1, v2 are colinear when projected on the plane z = 0
        const real det12xz = v1.x * v2.z - v1.z * v2.x;
        if (abs(det12xz) > 0.00000001f) {
            const real detv2cxz = c.x * v2.z - c.z * v2.x;
            const real l1xz = detv2cxz / det12xz;
            const real l2xz = (v1.x * c.z - v1.z * c.x) / det12xz;
            if (l1xz >= 0.0f && l2xz >= 0.0f && l1xz <= 1.0f && l1xz + l2xz <= 1.0f) {
                return t;
			}
            else {
                // The vectors v2, v3 are colinear when projected on the plane z = 0
                const real det23xz = v3.x * v2.z - v3.z * v2.x;
                // det23xz is necessarily != 0
                const real l1axz = detv2cxz / det23xz;
                if (l1axz >= 0.0f && l1axz <= 1.0f) {
                    const real l2axz = (v3.x * c.z - v3.z * c.x) / det23xz;
                    if (l2axz >= 0.0f && l1axz + l2axz <= 1.0f) {
                        return t;
                    }
                }
            }
		}
        else {
            // The vectors v1, v2 are colinear when projected on the planes y = 0 and z = 0
            const real det12yz = v1.y * v2.z - v1.z * v2.y;
            if (abs(det12yz) > 0.00000001f) {
                const real detv2cyz = c.y * v2.z - c.z * v2.y;
                const real l1yz = detv2cyz / det12yz;
                const real l2yz = (v1.y * c.z - v1.z * c.y) / det12yz;
                if (l1yz >= 0.0f && l2yz >= 0.0f && l1yz <= 1.0f && l1yz + l2yz <= 1.0f) {
                    return t;
                }
                else {
                    // The vectors v2, v3 are colinear when projected on the planes y = 0 and z = 0
                    const real det23yz = v3.y * v2.z - v3.z * v2.y;
                    // det23yz is necessarily != 0
                    const real l1ayz = detv2cyz / det23yz;
                    if (l1ayz >= 0.0f && l1ayz <= 1.0f) {
                        const real l2ayz = (v3.y * c.z - v3.z * c.y) / det23yz;
                        if (l2ayz >= 0.0f && l1ayz + l2ayz <= 1.0f) {
                            return t;
                        }
                    }
                }
            }
        }
    }

    return std::nullopt;
}

/* Returns the barycentric info (l1, l2, lower_triangle):
   (0 <= l1, l2 <= 1)
   p = position + l1 * v1 + l2 * v2 if lower_triangle == true,
   or
   p = position + l1 * v3 + l2 * v2 otherwise
*/
barycentric_info quad::get_barycentric(const rt::vector& p) const {

    const rt::vector c = p - position;
    const real det12xy = v1.x * v2.y - v1.y * v2.x;
    if (abs(det12xy) > 0.00000001f) {
        const real detv2cxy = c.x * v2.y - c.y * v2.x;
        const real l1 = detv2cxy / det12xy;
        const real l2 = (v1.x * c.y - v1.y * c.x) / det12xy;
        if (l1 >= 0.0f && l2 >= 0.0f && l1 <= 1.0f && l1 + l2 <= 1.0f) {
            return barycentric_info(l1, l2, true);
        }
        else {
            const real det23 = v3.x * v2.y - v3.y * v2.x;
            const real l1 = detv2cxy / det23;
            const real l2 = (v3.x * c.y - v3.y * c.x) / det23;
            return barycentric_info(l1, l2, false);
        }
    }
    else {
        const real det12xz = v1.x * v2.z - v1.z * v2.x;
        if (abs(det12xz) > 0.00000001f) {
            const real detv2cxz = c.x * v2.z - c.z * v2.x;
            const real l1 = detv2cxz / det12xz;
            const real l2 = (v1.x * c.z - v1.z * c.x) / det12xz;
            if (l1 >= 0.0f && l2 >= 0.0f && l1 <= 1.0f && l1 + l2 <= 1.0f) {
                return barycentric_info(l1, l2, true);
            }
            else {
                const real det23xz = v3.x * v2.z - v3.z * v2.x;
                const real l1 = detv2cxz / det23xz;
                const real l2 = (v3.x * c.z - v3.z * c.x) / det23xz;
                return barycentric_info(l1, l2, false);
            }
        }
        else {
            const real det12yz = v1.y * v2.z - v1.z * v2.y;
            const real detv2cyz = c.y * v2.z - c.z * v2.y;
            const real l1 = detv2cyz / det12yz;
            const real l2 = (v1.y * c.z - v1.z * c.y) / det12yz;
            if (l1 >= 0.0f && l2 >= 0.0f && l1 <= 1.0f && l1 + l2 <= 1.0f) {
                return barycentric_info(l1, l2, true);
            }
            else {
                const real det23yz = v3.y * v2.z - v3.z * v2.y;
                const real l1 = detv2cyz / det23yz;
                const real l2 = (v3.y * c.z - v3.z * c.y) / det23yz;
                return barycentric_info(l1, l2, false);
            }
        }
    }
}

rt::vector quad::get_interpolated_normal(const barycentric_info& bary) const {
    if (bary.lower_triangle) {
        return (((1.0f - bary.l1 - bary.l2) * vn0) + (bary.l1 * vn1) + (bary.l2 * vn2));
    }
    else {
        return (((1.0f - bary.l1 - bary.l2) * vn0) + (bary.l1 * vn3) + (bary.l2 * vn2));
    }
    
}

hit quad::compute_intersection(ray& r, const real t) const {

    // const rt::vector p1 = position + v1;
    // const rt::vector p2 = position + v2;
    // const rt::vector p3 = position + v3;
    // printf("\n\np0 = (%lf, %lf, %lf), p1 = (%lf, %lf, %lf), p2 = (%lf, %lf, %lf), p3 = (%lf, %lf, %lf)\n\n",
    //     position.x, position.y, position.z, p1.x, p1.y, p1.z, p2.x, p2.y, p2.z, p3.x, p3.y, p3.z);

    const rt::vector p = r.get_origin() + t * r.get_direction();
    const object* pt_obj = this;
    ray* pt_ray = &r;

#ifdef SMOOTH_SHADING
    
    // Computation of the interpolated normal vector
    const barycentric_info bary = get_barycentric(p);
    const bool inward = (r.get_direction() | normal) <= 0.0f;

    return hit(pt_ray, p, get_interpolated_normal(bary), pt_obj, inward);

#else // Flat shading
    
    return hit(pt_ray, p, normal, pt_obj);

#endif
}

/* Minimum and maximum coordinates */
min_max_coord quad::get_min_max_coord() const {

    const rt::vector p1 = position + v1;
    const rt::vector p2 = position + v2;
    const rt::vector p3 = position + v3;

    const real min_x = std::min(position.x, std::min(p1.x, std::min(p2.x, p3.x)));
    const real max_x = std::max(position.x, std::max(p1.x, std::max(p2.x, p3.x)));

    const real min_y = std::min(position.y, std::min(p1.y, std::min(p2.y, p3.y)));
    const real max_y = std::max(position.y, std::max(p1.y, std::max(p2.y, p3.y)));
    
    const real min_z = std::min(position.z, std::min(p1.z, std::min(p2.z, p3.z)));
    const real max_z = std::max(position.z, std::max(p1.z, std::max(p2.z, p3.z)));

    return min_max_coord(min_x, max_x, min_y, max_y, min_z, max_z);
}

/* Prints the quad */
void quad::print() const {
    printf("p0 = (%lf, %lf, %lf), ", position.x, position.y, position.z);
    rt::vector p1 = position + v1;
    printf("p1 = (%lf, %lf, %lf), ", p1.x, p1.y, p1.z);
    rt::vector p2 = position + v2;
    printf("p2 = (%lf, %lf, %lf)\n", p2.x, p2.y, p2.z);
    rt::vector p3 = position + v3;
    printf("p3 = (%lf, %lf, %lf)\n", p3.x, p3.y, p3.z);
}

/* Normal map vector computation at render time
    Local normal may be the normal of the triangle (for flat shading) or the smoothed normal, and in this case the tangent space should be reorthonormalized */
rt::vector quad::compute_normal_from_map(const rt::vector& tangent_space_normal, const rt::vector& local_normal) const {

#ifdef SMOOTH_SHADING
    const rt::vector& t = texture_information.value().tangent;
    // Recompute the tangent space with Gram-Schmidt's method
    const rt::vector t2 = (t - (t | local_normal) * local_normal).unit();
    const rt::vector b2 = t2 ^ local_normal;

    return tangent_space_normal.x * t2 + tangent_space_normal.y * b2 + tangent_space_normal.z * local_normal;
#else
    // Flat shading
    const rt::vector& t = texture_information.value().tangent;
    const rt::vector& b = texture_information.value().bitangent;

    return tangent_space_normal.x * t + tangent_space_normal.y * b + tangent_space_normal.z * local_normal;
#endif

}