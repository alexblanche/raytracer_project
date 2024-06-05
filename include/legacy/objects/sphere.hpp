#pragma once

#include "object.hpp"

#include "light/vector.hpp"
#include "legacy/light/hit.hpp"

class sphere : public object {
    
    private:

        real radius;

    public:

        /* Constructors */
        
        sphere();

        sphere(const rt::vector& center, const real& radius, const rt::color& col);

        
        /* Intersection determination */

        std::optional<real> measure_distance(const ray& r) const;

        hit compute_intersection(ray& r, const real& t) const;
};
