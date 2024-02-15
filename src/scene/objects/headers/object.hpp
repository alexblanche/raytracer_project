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
    
        static unsigned int counter;
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
};