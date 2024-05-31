#pragma once

#include "object.hpp"
#include "polygon.hpp"
#include "plane.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"

#include <optional>

class quad : public polygon {
    
    private:
    
        /* A quad is defined by a normal (unit) vector (a,b,c), and three (non-unit) vectors position, v1, v2, v3
           (when the quad is made up of four points P0, P1, P2, P3 position = P0, v1 = P1-P0 and v2 = P2-P0, v3 = P3-P0),
           such that v1, v3 represent adjacent edges of the quad.
           Vertex normals can be specified, but are optional.
           The d parameter, defining the plane of equation ax+by+cz+d = 0, is stored in order to speed-up the intersection calculations.
        */

        rt::vector normal, v1, v2, v3, vn0, vn1, vn2, vn3;
        double d;

    public:

        /* Constructors */

        quad();
        
        // Constructor from four points
        quad(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2, const rt::vector& p3,
            const unsigned int material_index);

        // Constructor from four points with vertex normals
        quad(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2, const rt::vector& p3,
            const rt::vector& vn0, const rt::vector& vn1, const rt::vector& vn2, const rt::vector& vn3,
            const unsigned int material_index);

        // Constructors for textured quads
        quad(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2, const rt::vector& p3,
            const unsigned int material_index, const texture_info& info);

        quad(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2, const rt::vector& p3,
            const rt::vector& vn0, const rt::vector& vn1, const rt::vector& vn2, const rt::vector& vn3,
            const unsigned int material_index, const texture_info& info);

        /* Intersection determination */

        std::optional<double> measure_distance(const ray& r) const;

        /* Writes the barycentric coordinates in variables l1, l2, and returns the boolean lower_triangle:
           (0 <= l1, l2 <= 1)
           p = position + l1 * v1 + l2 * v2 if lower_triangle == true,
           or
           p = position + l1 * v3 + l2 * v2 otherwise
        */
        barycentric_info get_barycentric(const rt::vector& p) const;
        
        rt::vector get_interpolated_normal(const barycentric_info& bary) const;

        hit compute_intersection(ray& r, const double& t) const;

        /* Minimum and maximum coordinates */
        min_max_coord get_min_max_coord() const;
};
