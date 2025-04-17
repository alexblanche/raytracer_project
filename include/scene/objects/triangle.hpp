#pragma once

#include "object.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"

#include <optional>

class triangle : public object {
    
    private:
        /* A triangle is defined by a normal (unit) vector (a,b,c), and three (non-unit) vectors position, v1, v2
           (when the triangle is three points P0, P1, P2, position = P0, v1 = P1-P0 and v2 = P2-P0).
           Vertex normals can be specified, but are optional.
           The d parameter, defining the plane of equation ax+by+cz+d = 0, is stored in order to speed-up the intersection calculations.

           We do not store a plane object as an attribute of the triangle because objects are automatically added to the object set
           and searched through for each ray-object intersection computation.
        */

        rt::vector normal, v1, v2, vn0, vn1, vn2;
        real d;

    public:

        /* Constructors */

        triangle();
        
        // Constructor from three points
        triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2, 
            const size_t material_index, const std::optional<texture_info>& info);

        // Constructor from three points with vertex normals
        triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2,
            const rt::vector& vn0, const rt::vector& vn1, const rt::vector& vn2,
            const size_t material_index, const std::optional<texture_info>& info);

        // Constructor from three points with normal mapping enabled
        triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2,
            const size_t material_index, const std::optional<texture_info>& info, const bool normal_mapping);

        // Constructor from three points with vertex normals and normal mapping enabled
        triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2,
            const rt::vector& vn0, const rt::vector& vn1, const rt::vector& vn2,
            const size_t material_index, const std::optional<texture_info>& info, const bool normal_mapping);

        /* Returns the barycenter of the triangle */
        rt::vector get_barycenter() const;


        /* Intersection determination */

        std::optional<real> measure_distance(const ray& r) const;

        /* Returns the barycentric info (l1, l2):
           p = position + l1 * v1 + l2 * v2
           (0 <= l1, l2 <= 1)
        */
        barycentric_info get_barycentric(const rt::vector& p) const;

        rt::vector get_interpolated_normal(const barycentric_info& bary) const;

        hit compute_intersection(ray& r, const real t) const;


        /* Minimum and maximum coordinates */
        min_max_coord get_min_max_coord() const;

        /* Prints the triangle */
        void print() const;

        /* Normal map vector computation at render time
        Local normal may be the normal of the triangle (for flat shading) or the smoothed normal, and in this case the tangent space should be reorthonormalized */
        rt::vector compute_normal_from_map(const rt::vector& tangent_space_normal, const rt::vector& local_normal) const;

        rt::vector sample(randomgen& rg) const;
        
        rt::vector sample_visible(randomgen& rg, const rt::vector& pt) const;
};
