#pragma once

#include "light/hit.hpp"
#include "scene/material/texture_info.hpp"
#include "scene/material/barycentric.hpp"
#include "auxiliary/randomgen.hpp"
#include "auxiliary/min_max_coord.hpp"

#include <limits>
#include <optional>

constexpr unsigned int EMPTY_INDEX = std::numeric_limits<unsigned int>::max();

/* Main class for objects of a scene
   Each object type is a derived class */

class object {
    
    protected:
        /* Position of the object (depends on the type of object) */
        rt::vector position;

        /* Index of the material in the material_set vector of the scene */
        const unsigned int material_index = EMPTY_INDEX;

        /* Contains a texture_info if the object is textured */
        const unsigned int texture_information_index = EMPTY_INDEX;

    public:

        constexpr object() {}
        constexpr object(const rt::vector& position, unsigned int material_index, unsigned int texture_info_index = EMPTY_INDEX)
            : position(position), material_index(material_index), texture_information_index(texture_info_index) {}

        object(const object&)            = delete;
        object(object&&)                 = delete;
        object& operator=(const object&) = delete;
        object& operator=(object&&)      = delete;

        virtual ~object() noexcept       = default;

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

        /* Interface */

        /* Intersection determination: returns infinity if no object is hit */
        virtual real measure_distance(const ray& r) const                               = 0;

        virtual hit compute_intersection(const ray& r, real t) const                    = 0;

        /* Returns the minimum and maximum coordinates of the object along the three axes */
        virtual min_max_coord get_min_max_coord() const                                 = 0;

        /* Returns the barycentric info for the object (depends on the object type) */
        virtual barycentric_info get_barycentric(const rt::vector& p) const             = 0;

        virtual rt::vector compute_normal_from_map(
            const rt::vector& tangent_space_normal,
            const rt::vector& local_normal,
            const texture_info& info
        ) const                                                                         = 0;

        /* Uniformly samples a point on the object */
        virtual rt::vector sample(const randomgen& rg) const                                  = 0;

        /* Uniformly samples a point on the object that is visible from pt */
        virtual rt::vector sample_visible(const randomgen& rg, const rt::vector& pt) const    = 0;
};