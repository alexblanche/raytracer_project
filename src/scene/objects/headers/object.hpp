#pragma once

#include "../../../light/headers/vector.hpp"
#include "../../../light/headers/hit.hpp"
#include "../../../light/headers/ray.hpp"
#include "../../material/headers/material.hpp"

class object {
    
    protected:
        
        rt::vector position;
        unsigned int index;
        material mat;

    public:
    
        /* Counter of objects created */
        static unsigned int counter;

        /* Set of all the objects in the scene */
        /* Storing pointers allow the overridden methods send and intersect (from sphere, plane, triangle...)
           to be executed instead of the base (object) one */
        static std::vector<const object*> set;

        /* Constructors */

        object();

        object(const rt::vector& pos, const material& m);

        /* Accessors */

        rt::vector get_position() const {
            return position;
        }

        unsigned int get_index() const {
            return index;
        }

        material get_material() const {
            return mat;
        }

        
        // These three functions are overridden by derived classes

        /* Intersection determination */
        virtual double measure_distance(const ray& r) const;

        virtual hit compute_intersection(const ray& r, const double t) const;

        /* Writes the minimum and maximum coordinates of the object on the three axes */
        virtual void min_max_coord(double& min_x, double& max_x,
            double& min_y, double& max_y, double& min_z, double& max_z) const;

        static hit find_closest_object(const ray& r);
};