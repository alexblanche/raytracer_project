#pragma once

#include "light/ray.hpp"
#include "auxiliary/min_max_coord.hpp"

/* Axis-aligned bounding box */

class aabb {
    public:
        rt::vector position; // position = center or corner
        rt::vector dims;

    public:

        enum class type {
            Center, Corner
        };
        using enum type;
        static constexpr type type_ = Center;

        inline static unsigned int cpt = 0;

        constexpr aabb(const rt::vector& position, const rt::vector& dims_)
            : position(position) {
                
            if constexpr (type_ == Corner)
                dims = dims_;
            else if constexpr (type_ == Center)
                dims = dims_ * 0.5_r;
            
            cpt++;
        }

        aabb(const min_max_coord& mmc) {

            const auto& [ min_x, max_x, min_y, max_y, min_z, max_z ] = mmc;
            const rt::vector min(min_x, min_y, min_z);
            const rt::vector max(max_x, max_y, max_z);

            if constexpr (type_ == Corner)
                position = min;
            else if constexpr (type_ == Center)
                position = (max + min) * 0.5_r;
            dims = (max - min) * 0.5_r;
        }

        aabb(aabb&&)            noexcept = default;
        aabb& operator=(aabb&&) noexcept = default;

        aabb(const aabb&)            = delete;
        aabb& operator=(const aabb&) = delete;
        
        ~aabb() noexcept {}

        static constexpr aabb infinite_box() {
            return aabb(rt::ZERO, rt::vector(infinity, infinity, infinity));
        }

        /* Only measures the distance from the outside of the aabb, otherwise returns 0.0_r */
        real measure_distance(const ray& r) const;

        inline bool is_hit_by(const ray& r) const {
            return measure_distance(r) < infinity;
        }

        /* Minimum and maximum coordinates */
        inline min_max_coord get_min_max_coord() const {
            if constexpr (type_ == Corner)
                return build_min_max_coord({ .min = position,        .max = position + dims });
            else if constexpr (type_ == Center)
                return build_min_max_coord({ .min = position - dims, .max = position + dims });
        }

        /* Returns the corner */
        inline rt::vector get_position() const {
            if constexpr (type_ == Corner)
                return position + dims;
            else if constexpr (type_ == Center)
                return position;
        }
};

static_assert(sizeof(aabb) == 2 * sizeof(rt::vector));
