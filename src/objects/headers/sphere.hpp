#pragma once

#include "object.hpp"

#include "../../screen/headers/color.hpp"
#include "../../light/headers/vector.hpp"
#include "../../light/headers/hit.hpp"

class sphere : public object {
    
    private:

        double radius;

    public:

        /* Constructors */

        sphere(const rt::vector& center, double radius, const rt::color& color);

        sphere();

        /* Accessors */

        rt::vector get_center() const;

        double get_radius() const;

        rt::color get_color() const;

        
        /* Intersection determination */

        double send(const ray& r) const;

        hit intersect(const ray& r, double t) const;
};
