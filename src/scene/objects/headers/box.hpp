#pragma once

#include "object.hpp"

#include "../../../light/headers/vector.hpp"
#include "../../../light/headers/hit.hpp"
#include "../../material/headers/material.hpp"

class box : public object {
    
    private:
    
        /* A box is defined by a vector position, which represents the center of the box,
           3 orthogonal unit vectors n1, n2, n3 representing an orthormal base orienting the box,
           and 3 doubles l1, l2, l3 representing the length of the box in the three directions (length, width and height)
        */

        rt::vector n1, n2, n3;
        double l1, l2, l3;

    public:

        /* Constructors */

        box();
        
        box(const rt::vector& center, const rt::vector& n1, const rt::vector& n2,
            const double l1, const double l2, const double l3, const material& material);

        box(const rt::vector& center, const rt::vector& n1, const rt::vector& n2,
            const double l1, const double l2, const double l3);
        
        /* Accessors */

        double get_l1() const;
        double get_l2() const;
        double get_l3() const;

        /* Intersection determination */

        double measure_distance(const ray& r) const;
        
        hit compute_intersection(const ray& r, const double t) const;

        /* Specific to (standard) boxes: returns true if the ray r hits the box
        The box is assumed to be standard (axes are n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1)) */
        bool is_hit_by(const ray& r) const;
};
