#pragma once

#include "object.hpp"

#include "../../../light/headers/vector.hpp"
#include "../../../light/headers/hit.hpp"
#include "../../material/headers/material.hpp"

class cylinder : public object {
    
    private:

        rt::vector direction;
        double radius, length;

    public:

        /* Constructors */
        
        cylinder();

        cylinder(const rt::vector& origin, const rt::vector& direction,
            const double radius, const double length,
            const material& material);

        /* Accessors */

        rt::vector get_origin() const {
            return position;
        }

        double get_radius() const {
            return radius;
        }

        double get_length() const {
            return length;
        }
        
        /* Intersection determination */

        double measure_distance(const ray& r) const;

        hit compute_intersection(const ray& r, const double t) const;

        /* Minimum and maximum coordinates */
        void min_max_coord(double& min_x, double& max_x,
            double& min_y, double& max_y, double& min_z, double& max_z) const;
};
