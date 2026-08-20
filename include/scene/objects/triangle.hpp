#pragma once

#include "scene/objects/object.hpp"
#include "scene/material/mapping_info.hpp"

#include <array>

constexpr real DET_EPSILON = 1.0e-10_r;

enum class det_case {
    XY, XZ, YZ, Error
};

inline real compute_det_2d(const rt::vector& v1, const rt::vector& v2, det_case case_det) {
    using enum det_case;
    switch (case_det) {
        case XY: return v1.x * v2.y - v1.y * v2.x;
        case XZ: return v1.x * v2.z - v1.z * v2.x;
        case YZ: return v1.y * v2.z - v1.z * v2.y;
        default: return 0.0_r;
    }
}

/*
inline std::string det_case_to_string(det_case case_det) {
    using enum det_case;
    switch (case_det) {
        case XY: return "XY";
        case XZ: return "XZ";
        case YZ: return "YZ";
        default: return "Error";
    }
}
*/

class triangle final : public object {
    
    private:
        /* A triangle is defined by a normal (unit) vector (a,b,c), and three (non-unit) vectors position, v1, v2
           (when the triangle is three points P0, P1, P2, position = P0, v1 = P1-P0 and v2 = P2-P0).
           Vertex normals can be specified, but are optional.
           The d parameter, defining the plane of equation ax+by+cz+d = 0, is stored in order to speed-up the intersection calculations.
        */

        rt::vector normal, v1, v2,
            vn0,
            dvn1, dvn2; // dvni = vni - vn0
        real d;
        real det;
        det_case case_det;

    public:

        class orientation final : public mapping_info {
            public:
                uvcoord uv[3];
                rt::vector tangent;
                rt::vector bitangent;

                orientation(const mapping::index_type index,
                    const std::array<uvcoord, 3>& uvc,
                    const rt::vector& v1, const rt::vector& v2);
        };
        
        // Constructor from three points
        triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2, 
            unsigned int material_index, unsigned int orientation_info_index = EMPTY_INDEX);

        // Constructor from three points with vertex normals
        triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2,
            const rt::vector& vn0init, const rt::vector& vn1, const rt::vector& vn2,
            unsigned int material_index, unsigned int orientation_info_index = EMPTY_INDEX);

        triangle(triangle&&) noexcept        = default;
        triangle(const triangle&)            = delete;
        triangle& operator=(const triangle&) = delete;
        triangle& operator=(triangle&&)      = delete;

        /* Returns the barycenter of the triangle */
        rt::vector get_barycenter() const;

        // Ugly, but quick fix for obj_parser (v1, v2 needed to construct orientation)
        std::pair<const rt::vector&, const rt::vector&> get_v1_v2() const {
            return { v1, v2 };
        }

        /* Intersection determination */

        real measure_distance(const ray& r) const override;

        uvcoord compute_uv(const rt::vector& p, const mapping_info* orientation_info) const override;

        hit compute_intersection(const ray& r, real t) const override;

        /* Minimum and maximum coordinates */
        min_max_coord get_min_max_coord() const override;

        /* Normal map vector computation at render time
        Local normal may be the normal of the triangle (for flat shading) or the smoothed normal, and in this case the tangent space should be reorthonormalized */
        rt::vector compute_normal_from_map(
            const rt::vector& tangent_space_normal,
            const rt::vector& local_normal,
            const mapping_info* orientation_info
        ) const override;

        /* General function, used in triangle and quad classes */
        static inline rt::vector sample_triangle(const randomgen& rg, const rt::vector& v0, const rt::vector& v1, const rt::vector& v2) {
            
            const real x = rg.random_ratio();
            const real y = rg.random_ratio();
            
            real u, v;
            if (x < y) {
                u = x / 2;
                v = y - u;
            }
            else {
                v = y / 2;
                u = x - v;
            }

            return fma(v1, u, fma(v2, v, v0));
        }

        rt::vector sample(const randomgen& rg) const override;
        
        rt::vector sample_visible(const randomgen& rg, const rt::vector& pt) const override;

        void print() const override;

    private:
        
        stcoord compute_st(const rt::vector& p) const;

        rt::vector compute_interpolated_normal(const stcoord& st) const;
};
