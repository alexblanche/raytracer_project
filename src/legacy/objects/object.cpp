#include "legacy/objects/object.hpp"

/* Constructors */

object::object()
    : position(rt::vector()), color(rt::color(255, 255, 255)) {}

object::object(const rt::vector& position, const rt::color& col)
    : position(position), color(col) {}


/* Default versions: these four functions are overridden by derived classes */

/* Intersection determination */

std::optional<real> object::measure_distance(const ray& /*r*/) const {
    return std::nullopt;
}

hit object::compute_intersection(ray& /*r*/, const real& /*t*/) const {
    return hit();
}