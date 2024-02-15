#pragma once

#include "object.hpp"
#include "plane.hpp"

#include "../../../light/headers/vector.hpp"
#include "../../../light/headers/hit.hpp"
#include "../../material/headers/material.hpp"

class triangle : public object {
    
    private:
    
        /* A triangle is defined by a normal (unit) vector (a,b,c), and three (non-unit) vectors position, v1, v2
           (when the triangle is three points P0, P1, P2, position = P0, v1 = P1-P0 and v2 = P2-P0).
           Vertex normals can be specified, but are optional
           the d parameter, defining the plane of equation ax+by+cz+d = 0, is stored in order to speed-up the intersection calculations

           We do not store a plane object as an attribute of the triangle because objects are automatically added to the object set
           and searched through for each ray-object intersection computation.
        */

        rt::vector normal, v1, v2, vn0, vn1, vn2;
        double d;

    public:

        /* Constructors */

        triangle();
        
        // Constructor from three points
        triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2, 
            const material& material);

        // Constructor from three points with vertex normals
        triangle(const rt::vector& p0, const rt::vector& p1, const rt::vector& p2,
            const rt::vector& vn0, const rt::vector& vn1, const rt::vector& vn2,
            const material& material);

        /* Accessors */

        rt::vector get_normal() const;


        /* Intersection determination */

        double measure_distance(const ray& r) const;

        /* Returns a vector (only the first two coordinates matter) with the barycentric coordinates (l1, l2):
           p = position + l1 * v1 + l2 * v2
           (0 <= l1, l2 <= 1)
        */
        rt::vector get_barycentric(const rt::vector& p) const;

        rt::vector get_interpolated_normal(const double& l1, const double& l2) const;

        hit compute_intersection(const ray& r, const double t) const;
};