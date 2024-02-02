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

        sphere(const rt::vector& center, const double radius, const unsigned int index,
            const material& material);

        sphere();


        /* Accessors */

        rt::vector get_center() const;

        double get_radius() const;

        
        /* Intersection determination */

        double measure_distance(const ray& r) const;

        hit compute_intersection(const ray& r, const double t) const;
};