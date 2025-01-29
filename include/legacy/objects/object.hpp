#pragma once

#include "light/vector.hpp"
#include "legacy/light/hit.hpp"
#include "screen/color.hpp"

#include <optional>

class object {
    
    protected:
        /* Position of the object (depends on the type of object) */
        rt::vector position;

        /* Color of the object */
        const rt::color color;

    public:

        virtual ~object() {}

        /* Constructors */

        object();

        object(const rt::vector& pos, const rt::color& col);

        /* Accessors */

        inline rt::vector get_position() const {
            return position;
        }

        inline rt::color get_color() const {
            return color;
        }
 
        // These four functions are overridden by derived classes

        /* Intersection determination */
        virtual std::optional<real> measure_distance(const ray& r) const;

        virtual hit compute_intersection(ray& r, const real t) const;
};