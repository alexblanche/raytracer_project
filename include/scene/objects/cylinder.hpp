#pragma once

#include "object.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"

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

        double measure_distance(const ray& r) const;

        hit compute_intersection(ray& r, const double t) const;

        /* Minimum and maximum coordinates */
        void min_max_coord(double& min_x, double& max_x,
            double& min_y, double& max_y, double& min_z, double& max_z) const;
};
