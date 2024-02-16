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

        rt::vector get_position() const;

        unsigned int get_index() const;

        material get_material() const;

        /* Intersection determination */
        // These two functions are overridden by derived classes

        virtual double measure_distance(const ray& r) const;

        virtual hit compute_intersection(const ray& r, const double t) const;

        static hit find_closest_object(const ray& r);
};