#pragma once

#include "object.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"

#include <optional>

class quad : public object {
    
    private:
    
        /* A quad is defined by a normal (unit) vector (a,b,c), and three (non-unit) vectors position, v1, v2, v3
           (when the quad is made up of four points P0, P1, P2, P3 position = P0, v1 = P1-P0 and v2 = P2-P0, v3 = P3-P0),
           such that v1, v3 represent adjacent edges of the quad.
           Vertex normals can be specified, but are optional.
           The d parameter, defining the plane of equation ax+by+cz+d = 0, is stored in order to speed-up the intersection calculations.
        */

        rt::vector normal, v1, v2, v3, vn0, vn1, vn2, vn3;
        real d;

    public:

        /* Constructors */

        quad();
        
        // Constructor from four points
        quad(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2, const rt::vector& p3,
            const size_t material_index, const std::optional<texture_info>& info);

        // Constructor from four points with vertex normals
        quad(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2, const rt::vector& p3,
            const rt::vector& vn0, const rt::vector& vn1, const rt::vector& vn2, const rt::vector& vn3,
            const size_t material_index, const std::optional<texture_info>& info);

        // Constructor from four points with normal mapping enabled
        quad(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2, const rt::vector& p3,
            const size_t material_index, const std::optional<texture_info>& info, const bool normal_mapping);

        // Constructor from four points with vertex normals and normal mapping enabled
        quad(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2, const rt::vector& p3,
            const rt::vector& vn0, const rt::vector& vn1, const rt::vector& vn2, const rt::vector& vn3,
            const size_t material_index, const std::optional<texture_info>& info, const bool normal_mapping);

        /* Returns barycenter of the quad */
        rt::vector get_barycenter() const;

        /* Intersection determination */

        std::optional<real> measure_distance(const ray& r) const;

        /* Returns the barycentric info (l1, l2, lower_triangle):
           (0 <= l1, l2 <= 1)
           p = position + l1 * v1 + l2 * v2 if lower_triangle == true,
           or
           p = position + l1 * v3 + l2 * v2 otherwise
        */
        barycentric_info get_barycentric(const rt::vector& p) const;
        
        rt::vector get_interpolated_normal(const barycentric_info& bary) const;

        hit compute_intersection(ray& r, const real t) const;

        /* Minimum and maximum coordinates */
        min_max_coord get_min_max_coord() const;

        /* Prints the quad */
        void print() const;

        /* Normal map vector computation at render time
        Local normal may be the normal of the quad (for flat shading) or the smoothed normal, and in this case the tangent space should be reorthonormalized */
        rt::vector compute_normal_from_map(const rt::vector& tangent_space_normal, const rt::vector& local_normal) const;
};
