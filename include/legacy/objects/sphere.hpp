#pragma once

#include "object.hpp"

#include "light/vector.hpp"
#include "legacy/light/hit.hpp"

class sphere : public object {
    
    private:

        double radius;

    public:

        /* Constructors */
        
        sphere();

        sphere(const rt::vector& center, const double& radius, const rt::color& col);

        
        /* Intersection determination */

        std::optional<double> measure_distance(const ray& r) const;

        hit compute_intersection(ray& r, const double& t) const;
};
