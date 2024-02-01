#pragma once

#include "../../screen/headers/color.hpp"
#include "../../light/headers/vector.hpp"
#include "../../light/headers/hit.hpp"
#include "../../light/headers/ray.hpp"

class object {
    protected:
        rt::vector position;
        rt::color color;
        // + material
        unsigned int index;

    public:
        /* Constructors */

        object();

        object(const rt::vector& pos, const rt::color& col, const unsigned int i);

        /* Accessors */

        rt::vector get_position() const;

        rt::color get_color() const;

        unsigned int get_index() const;

        /* Intersection determination */
        // These two functions are overridden by derived classes

        virtual double send(const ray& r) const;

        virtual hit intersect(const ray& r, const double t) const;
};