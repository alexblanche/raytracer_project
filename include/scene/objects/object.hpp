#pragma once

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "light/ray.hpp"
#include "scene/material/material.hpp"

class object {
    
    protected:
        rt::vector position;
        const material* mat;

    public:

        virtual ~object() {}

        /* Constructors */

        object();

        object(const rt::vector& pos, const material* m);

        /* Accessors */

        inline rt::vector get_position() const {
            return position;
        }

        inline const material get_material() const {
            return *mat;
        }

        
        // These four functions are overridden by derived classes

        /* Intersection determination */
        virtual double measure_distance(const ray& r) const;

        virtual hit compute_intersection(ray& r, const double t) const;

        /* Writes the barycentric coordinates in variables l1, l2
           The boolean return value is used for determining the three points considered in quads */
        virtual bool get_barycentric(const rt::vector& p, double& l1, double& l2) const;

        /* Writes the minimum and maximum coordinates of the object on the three axes */
        virtual void min_max_coord(double& min_x, double& max_x,
            double& min_y, double& max_y, double& min_z, double& max_z) const;
};