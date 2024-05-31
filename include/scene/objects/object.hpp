#pragma once

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "light/ray.hpp"
#include "scene/material/material.hpp"

#include <optional>

class object {
    
    protected:
        /* Position of the object (depends on the type of object) */
        rt::vector position;

        /* Index of the material in the material_set vector of the scene */
        const unsigned int material_index;

        /* True if the object is textured (only allowed when it is a triangle or quad) */
        const bool textured;

    public:

        virtual ~object() {}

        /* Constructors */

        object();

        object(const rt::vector& pos, const unsigned int material_index);

        object(const rt::vector& pos, const unsigned int material_index, const bool textured);

        /* Accessors */

        inline rt::vector get_position() const {
            return position;
        }

        inline unsigned int get_material_index() const {
            return material_index;
        }

        inline bool is_textured() const {
            return textured;
        }

        
        // These four functions are overridden by derived classes

        /* Intersection determination */
        virtual std::optional<double> measure_distance(const ray& r) const;

        virtual hit compute_intersection(ray& r, const double& t) const;

        /* Writes the minimum and maximum coordinates of the object on the three axes */
        virtual void min_max_coord(double& min_x, double& max_x,
            double& min_y, double& max_y, double& min_z, double& max_z) const;
};