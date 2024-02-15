#pragma once

#include "object.hpp"
#include "plane.hpp"

#include "../../../light/headers/vector.hpp"
#include "../../../light/headers/hit.hpp"
#include "../../material/headers/material.hpp"

class triangle : public object {
    
    private:
    
        /* A triangle is defined by a plane of equation (P): ax + by + cz + d = 0
           three (non-unit) vectors v1, v2 (when the triangle is three points P1, P2, P3, position = P1, v1 = P2-P1 and v2 = P3-P1),
        */

        plane p;
        rt::vector v1, v2;

    public:

        /* Constructors */

        triangle();
        
        // Constructor from the stored the parameters
        triangle(const plane& p, const rt::vector& position, const rt::vector& v1, const rt::vector& v2,
            const material& material);
        
        // Constructor from three points
        triangle(const rt::vector& p1, const rt::vector& p2, const rt::vector& p3, 
            const material& material);

        /* Accessors */

        rt::vector get_normal() const;


        /* Intersection determination */

        double measure_distance(const ray& r) const;
        
        hit compute_intersection(const ray& r, const double t) const;

        /* Returns a vector (only the first two coordinates matter) with the barycentric coordinates (l1, l2):
           p = position + l1 * v1 + l2 * v2
        */
        rt::vector get_barycentric(const rt::vector& p) const;
};
