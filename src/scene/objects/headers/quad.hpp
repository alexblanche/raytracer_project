#pragma once

#include "object.hpp"
#include "plane.hpp"

#include "../../../light/headers/vector.hpp"
#include "../../../light/headers/hit.hpp"
#include "../../material/headers/material.hpp"

class quad : public object {
    
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
            const material& material);

        // Constructor from four points with vertex normals
        quad(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2, const rt::vector& p3,
            const rt::vector& vn0, const rt::vector& vn1, const rt::vector& vn2, const rt::vector& vn3,
            const material& material);

        /* Accessors */

        rt::vector get_normal() const;

        rt::vector get_v1() const;
        rt::vector get_v2() const;
        rt::vector get_v3() const;


        /* Intersection determination */

        double measure_distance(const ray& r) const;

        /* Writes the barycentric coordinates in variables l1, l2, and returns the boolean triangle:
           (0 <= l1, l2 <= 1)
           p = position + l1 * v1 + l2 * v2 if triangle == true,
           or
           p = position + l1 * v3 + l2 * v2 otherwise
        */
        bool get_barycentric(const rt::vector& p, double& l1, double& l2) const;
            
        rt::vector get_interpolated_normal(const double& l1, const double& l2, const bool triangle) const;

        hit compute_intersection(const ray& r, const double t) const;
};