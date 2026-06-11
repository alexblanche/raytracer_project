#pragma once

#include "legacy/objects/legacy_object.hpp"

class sphere : public object {
    
    private:

        real radius;

    public:

        /* Constructors */
        
        sphere();

        sphere(const rt::vector& center, real radius, const rt::color& col);

        
        /* Intersection determination */

        std::optional<real> measure_distance(const ray& r) const;

        hit compute_intersection(ray& r, real t) const;
};
