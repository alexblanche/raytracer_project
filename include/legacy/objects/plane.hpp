#pragma once

#include "object.hpp"

#include "light/vector.hpp"
#include "legacy/light/hit.hpp"

class plane : public object {
    
    private:

        double a, b, c, d;

        /* A plane (P) of equation (P): ax + by + cz + d = 0
         defined by 4 doubles a,b,c,d */

    public:

        /* Constructors */

        plane();
        
        plane(const double& sa, const double& sb, const double& sc, const double& sd,
            const rt::color& col);
        
        plane(const double& pa, const double& pb, const double& pc, const rt::vector& position,
            const rt::color& col);

        /* Accessors */

        inline rt::vector get_normal() const {
            return rt::vector(a, b, c);
        }


        /* Intersection determination */

        std::optional<double> measure_distance(const ray& r) const;
        
        hit compute_intersection(ray& r, const double& t) const;
};
