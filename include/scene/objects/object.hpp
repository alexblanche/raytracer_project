#pragma once

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "light/ray.hpp"
#include "scene/material/material.hpp"
#include "scene/material/texture.hpp"
#include "scene/material/texture_info.hpp"
#include "scene/material/barycentric.hpp"

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

#define EMPTY_INDEX ((unsigned int) -1)


/* Main class for objects of a scene
   Each object type is a derived class */

class object {
    
    protected:
        /* Position of the object (depends on the type of object) */
        rt::vector position;

        /* Contains a texture_info if the object is textured */
        const unsigned int texture_information_index;

        /* Index of the material in the material_set vector of the scene */
        const unsigned int material_index;

    public:

        virtual ~object() {}

        /* Constructors */

        object();

        object(const rt::vector& pos, const unsigned int material_index);

        object(const rt::vector& pos, const unsigned int material_index, const unsigned int texture_info_index);

        /* Accessors */

        inline const rt::vector& get_position() const {
            return position;
        }

        inline unsigned int get_material_index() const {
            return material_index;
        }

        inline bool is_textured() const {
            return texture_information_index != EMPTY_INDEX;
        }

        inline unsigned int get_texture_info_index() const {
            return texture_information_index;
        }

        
        // These four functions are overridden by derived classes

        /* Intersection determination */
        virtual std::optional<real> measure_distance(const ray& r) const;

        virtual hit compute_intersection(ray& r, const real t) const;

        /* Returns the minimum and maximum coordinates of the object along the three axes */
        virtual min_max_coord get_min_max_coord() const;

        /* Returns the barycentric info for the object (depends on the object type) */
        virtual barycentric_info get_barycentric(const rt::vector& p) const;

        virtual rt::vector compute_normal_from_map(const rt::vector& tangent_space_normal, const rt::vector& local_normal) const;

        /* Sampling */

        /* Uniformly samples a point on the object */
        virtual rt::vector sample(randomgen& rg) const;

        /* Uniformly samples a point on the object that is visible from pt */
        virtual rt::vector sample_visible(randomgen& rg, const rt::vector& pt) const;
};