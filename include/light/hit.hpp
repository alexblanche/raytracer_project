#pragma once

#include "light/ray.hpp"

/** The hit class contains the information
 * of a ray hitting a surface: the ray in question,
 * the point of contact, the normal of the surface at
 * this point and a pointer to the object hit.
*/

/* Forward-declaring the object class, to solve mutual recursivity between the hit and object classes */
class object;

enum class orientation_type {
    Inward, Outward
};

static inline orientation_type get_orientation(const rt::vector& direction, const rt::vector& normal) {
    using enum orientation_type;
    return (direction | normal) <= 0.0f ? Inward : Outward;
}

class hit {
    private:
        rt::vector point;
        rt::vector normal;
        const object* hit_object;
        orientation_type orientation;

    public:
        hit(const rt::vector& point, const rt::vector& normal, const object* hit_object, const orientation_type orientation)
            : point(point), normal(normal), hit_object(hit_object), orientation(orientation) {}

        using enum orientation_type;
        hit(const rt::vector& direction, const rt::vector& point, const rt::vector& normal, const object* hit_object)
            : hit(point, normal, hit_object, get_orientation(direction, normal)) {}

        hit(hit&&) noexcept        = default;
        hit(const hit&)            = delete;
        hit& operator=(const hit&) = delete;
        hit& operator=(hit&&)      = delete;

        inline const rt::vector& get_point() const {
            return point;
        }
        
        inline const rt::vector& get_normal() const {
            return normal;
        }

        inline const object* get_object() const {
            return hit_object;
        }

        inline orientation_type is_inward() const {
            return orientation;
        }
};