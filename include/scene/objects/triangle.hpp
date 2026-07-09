#pragma once

#include "scene/objects/object.hpp"

enum class det_case {
    XY, XZ, YZ, Error
};

inline real compute_det_2d(const rt::vector& v1, const rt::vector& v2, det_case case_det) {
    using enum det_case;
    switch (case_det) {
        case XY:    return v1.x * v2.y - v1.y * v2.x;
        case XZ:    return v1.x * v2.z - v1.z * v2.x;
        case YZ:    return v1.y * v2.z - v1.z * v2.y;
        case Error: return 0.0_r;
    }
}

class triangle : public object {
    
    private:
        /* A triangle is defined by a normal (unit) vector (a,b,c), and three (non-unit) vectors position, v1, v2
           (when the triangle is three points P0, P1, P2, position = P0, v1 = P1-P0 and v2 = P2-P0).
           Vertex normals can be specified, but are optional.
           The d parameter, defining the plane of equation ax+by+cz+d = 0, is stored in order to speed-up the intersection calculations.

           We do not store a plane object as an attribute of the triangle because objects are automatically added to the object set
           and searched through for each ray-object intersection computation.
        */

        rt::vector normal, v1, v2, vn0, vn1mvn0, vn2mvn0;
        real d;
        real det;
        det_case case_det;


    public:
        
        // Constructor from three points
        triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2, 
            unsigned int material_index, unsigned int texture_info_index = EMPTY_INDEX);

        // Constructor from three points with vertex normals
        triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2,
            const rt::vector& vn0, const rt::vector& vn1, const rt::vector& vn2,
            unsigned int material_index, unsigned int texture_info_index = EMPTY_INDEX);

        // Constructor from three points with normal mapping enabled
        triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2,
            unsigned int material_index, unsigned int texture_info_index,
            texture_info& info);

        // Constructor from three points with vertex normals and normal mapping enabled
        triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2,
            const rt::vector& vn0, const rt::vector& vn1, const rt::vector& vn2,
            unsigned int material_index, unsigned int texture_info_index,
            texture_info& info);

        triangle(triangle&&) noexcept        = default;
        triangle(const triangle&)            = delete;
        triangle& operator=(const triangle&) = delete;
        triangle& operator=(triangle&&)      = delete;

        /* Returns the barycenter of the triangle */
        rt::vector get_barycenter() const;


        /* Intersection determination */

        real measure_distance(const ray& r) const final;

        /* Returns the barycentric info (l1, l2):
           p = position + l1 * v1 + l2 * v2
           (0 <= l1, l2 <= 1)
        */
        barycentric_info get_barycentric(const rt::vector& p) const final;

        rt::vector get_interpolated_normal(const barycentric_info& bary) const;

        hit compute_intersection(const ray& r, real t) const final;


        /* Minimum and maximum coordinates */
        min_max_coord get_min_max_coord() const final;

        /* Prints the triangle */
        void print() const;

        /* Normal map vector computation at render time
        Local normal may be the normal of the triangle (for flat shading) or the smoothed normal, and in this case the tangent space should be reorthonormalized */
        rt::vector compute_normal_from_map(const rt::vector& tangent_space_normal, const rt::vector& local_normal, const texture_info& info) const final;

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

        rt::vector sample(const randomgen& rg) const final;
        
        rt::vector sample_visible(const randomgen& rg, const rt::vector& pt) const final;
};
