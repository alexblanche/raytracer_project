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
        /* Constructors */

        object();

        object(const rt::vector& pos, const unsigned int i, const material& m);

        /* Accessors */

        rt::vector get_position() const;

        unsigned int get_index() const;

        material get_material() const;

        /* Intersection determination */
        // These two functions are overridden by derived classes

        virtual double send(const ray& r) const;

        virtual hit intersect(const ray& r, const double t) const;
};