#pragma once

#include "light/ray.hpp"
#include "auxiliary/min_max_coord.hpp"

/* Axis-aligned bounding box */

class aabb {
    private:
        rt::vector position; // position = center if type = Center, or corner if type = Corner
        rt::vector dims;

    public:

        enum class type {
            Center, Corner
        };
        using enum type;
        static constexpr type type = Center;

        inline static unsigned int cpt = 0;

        aabb(const rt::vector& position, const rt::vector& dims_)
            : position(position) {
                
            if constexpr (type == Corner)
                dims = dims_;
            else if constexpr (type == Center)
                dims = dims_ * 0.5_r;
            
            cpt++;
        }

        aabb(aabb&&)                 = delete;
        aabb(const aabb&)            = delete;
        aabb& operator=(const aabb&) = delete;
        aabb& operator=(aabb&&)      = delete;

        static constexpr aabb infinite_box() {
            return aabb(ZERO, rt::vector(infinity, infinity, infinity));
        }

        /* Only measures the distance from the outside of the aabb, otherwise returns 0.0_r */
        real measure_distance(const ray& r) const;

        inline bool is_hit_by(const ray& r) const {
            return measure_distance(r) < infinity;
        }

        /* Minimum and maximum coordinates */
        inline min_max_coord get_min_max_coord() const {
            if constexpr (type == type::Corner)
                return build_min_max_coord(position, position + dims);
            else if constexpr (type == Center)
                return build_min_max_coord(position - dims, position + dims);
        }

        /* Returns the corner */
        inline rt::vector get_position() const {
            if constexpr (type == Corner)
                return position + dims;
            else if constexpr (type == Center)
                return position;
        }
};

static_assert(sizeof(aabb) == 2 * sizeof(rt::vector));
