#pragma once

#include "object.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"

#include <optional>

class cylinder : public object {
    
    private:

        rt::vector direction;
        double radius, length;

    public:

        /* Constructors */
        
        cylinder();

        cylinder(const rt::vector& origin, const rt::vector& direction,
            const double& radius, const double& length,
            const unsigned int material_index);

        /* Accessors */

        inline rt::vector get_origin() const {
            return position;
        }

        inline double get_radius() const {
            return radius;
        }

        inline double get_length() const {
            return length;
        }
        
        /* Intersection determination */

        std::optional<double> measure_distance(const ray& r) const;

        hit compute_intersection(ray& r, const double& t) const;

        /* Minimum and maximum coordinates */
        min_max_coord get_min_max_coord() const;
};
