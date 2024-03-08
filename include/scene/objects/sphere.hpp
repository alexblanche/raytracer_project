#pragma once

#include "object.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"

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

        /* Minimum and maximum coordinates */
        void min_max_coord(double& min_x, double& max_x,
            double& min_y, double& max_y, double& min_z, double& max_z) const;
};
