#pragma once

#include "object.hpp"
#include "polygon.hpp"
#include "plane.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"

#include <optional>

class triangle : public polygon {
    
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

        /* Returns the barycenter of the triangle */
        rt::vector get_barycenter() const;


        /* Intersection determination */

        std::optional<real> measure_distance(const ray& r) const;

        /* Writes the barycentric coordinates in variables l1, l2:
           p = position + l1 * v1 + l2 * v2
           (0 <= l1, l2 <= 1)
           The boolean return value is unused for triangles (only for quads).
        */
        barycentric_info get_barycentric(const rt::vector& p) const;

        rt::vector get_interpolated_normal(const barycentric_info& bary) const;

        hit compute_intersection(ray& r, const real t) const;


        /* Minimum and maximum coordinates */
        min_max_coord get_min_max_coord() const;

        /* Prints the triangle */
        void print() const;
};
