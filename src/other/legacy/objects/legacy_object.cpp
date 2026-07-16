#include "other/legacy/objects/legacy_object.hpp"

/* Default versions: these four functions are overridden by derived classes */

/* Intersection determination */

std::optional<real> object::measure_distance(const ray& /*r*/) const {
    return std::nullopt;
}

hit object::compute_intersection(ray& /*r*/, const real /*t*/) const {
    return hit();
}