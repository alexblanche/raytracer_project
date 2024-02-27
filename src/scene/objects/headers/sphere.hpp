#pragma once

#include "object.hpp"

#include "../../../light/headers/vector.hpp"
#include "../../../light/headers/hit.hpp"
#include "../../material/headers/material.hpp"

class sphere : public object {
    
    private:

        double radius;

    public:

        /* Constructors */
        
        sphere();

        sphere(const rt::vector& center, const double radius, const material& material);

        /* Accessors */

        rt::vector get_center() const {
            return position;
        }

        double get_radius() const {
            return radius;
        }

        
        /* Intersection determination */

        double measure_distance(const ray& r) const;

        hit compute_intersection(const ray& r, const double t) const;
};
