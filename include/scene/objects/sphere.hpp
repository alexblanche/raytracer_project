#pragma once

#include "object.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"

#include <optional>

class sphere : public object {
    
    private:

        real radius;

    public:

        /* Constructors */
        
        sphere();

        sphere(const rt::vector& center, const real& radius, const size_t material_index);

        
        /* Intersection determination */

        std::optional<real> measure_distance(const ray& r) const;

        hit compute_intersection(ray& r, const real& t) const;

        /* Minimum and maximum coordinates */
        min_max_coord get_min_max_coord() const;
};
