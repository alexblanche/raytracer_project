#pragma once

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "light/ray.hpp"
#include "scene/material/material.hpp"

#include <optional>

/* Output struct from min_max_coord */
struct min_max_coord {
    real min_x, max_x, min_y, max_y, min_z, max_z;

    min_max_coord(const real min_x, const real max_x,
        const real min_y, const real max_y,
        const real min_z, const real max_z)

        : min_x(min_x), max_x(max_x),
        min_y(min_y), max_y(max_y), 
        min_z(min_z), max_z(max_z) {}

    min_max_coord() {}
};


/* Main class for objects of a scene
   Each object type is a derived class */

class object {
    
    protected:
        /* Position of the object (depends on the type of object) */
        rt::vector position;

        /* Index of the material in the material_set vector of the scene */
        const size_t material_index;

        /* True if the object is textured (only allowed when it is a triangle or quad) */
        const bool textured;

    public:

        virtual ~object() {}

        /* Constructors */

        object();

        object(const rt::vector& pos, const size_t material_index);

        object(const rt::vector& pos, const size_t material_index, const bool textured);

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
        virtual std::optional<real> measure_distance(const ray& r) const;

        virtual hit compute_intersection(ray& r, const real t) const;

        /* Writes the minimum and maximum coordinates of the object on the three axes */
        virtual min_max_coord get_min_max_coord() const;
};