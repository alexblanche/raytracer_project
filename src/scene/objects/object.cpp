#include "scene/objects/object.hpp"
#include "scene/material/material.hpp"

#include <optional>

/* Constructors */

object::object()
    : position(rt::vector()), material_index((size_t) -1),
      textured(false) {}

object::object(const rt::vector& position, const size_t material_index)
    : position(position), material_index(material_index),
      textured(false) {}

object::object(const rt::vector& position, const size_t material_index, const bool textured)
    : position(position), material_index(material_index),
      textured(textured) {}



/* Default versions: these four functions are overridden by derived classes */

/* Intersection determination */

std::optional<real> object::measure_distance(const ray& /*r*/) const {
    return std::nullopt;
}

hit object::compute_intersection(ray& /*r*/, const real /*t*/) const {
    return hit();
}

/* Writes the minimum and maximum coordinates of the object on the three axes */
min_max_coord object::get_min_max_coord() const {

    return min_max_coord();
}
