#pragma once

#include "object.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"

class box : public object {
    
    private:
    
        /* A box is defined by a vector position, which represents the center of the box,
           3 orthogonal unit vectors n1, n2, n3 representing an orthormal base orienting the box,
           and 3 doubles l1, l2, l3 representing the length of the box in the three directions (length, width and height)
        */

        rt::vector n1, n2, n3;
        double l1, l2, l3;

    public:

        /* Constructors */

        box();
        
        /* Main constructor */
        box(const rt::vector& center, const rt::vector& n1, const rt::vector& n2,
            const double l1, const double l2, const double l3, const material& material);

        /* Constructor used for bounding boxes: no material and no index */
        box(const rt::vector& center, const rt::vector& n1, const rt::vector& n2,
            const double l1, const double l2, const double l3);
        
        /* Accessors */

        double get_l1() const {
            return l1;
        }
        double get_l2() const {
            return l2;
        }
        double get_l3() const {
            return l3;
        }

        /* Intersection determination */

        double measure_distance(const ray& r) const;
        
        hit compute_intersection(const ray& r, const double t) const;

        /* Minimum and maximum coordinates */
        void min_max_coord(double& min_x, double& max_x,
            double& min_y, double& max_y, double& min_z, double& max_z) const;

        /* Specific to (standard) boxes: returns true if the ray r hits the box
        The box is assumed to be standard (axes are n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1)) */
        bool is_hit_by(const ray& r) const;
};
