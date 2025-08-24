#include "scene/objects/object.hpp"
#include "scene/material/material.hpp"

#include <optional>

/* Constructors */

object::object()
    : position(rt::vector()),
        texture_information_index(EMPTY_INDEX),
        material_index(EMPTY_INDEX) {}

object::object(const rt::vector& position, const unsigned int material_index)
    : position(position),
        texture_information_index(EMPTY_INDEX),
        material_index(material_index) {}

object::object(const rt::vector& position, const unsigned int material_index, const unsigned int texture_info_index)
    : position(position),
        texture_information_index(texture_info_index),
        material_index(material_index) {}



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

barycentric_info object::get_barycentric(const rt::vector& /*p*/) const {

    return barycentric_info(0.0f, 0.0f);
}

rt::vector object::compute_normal_from_map(const rt::vector& /*tangent_space_normal*/,
    const rt::vector& local_normal) const {
    
    return local_normal;
}

/* Uniformly samples a point on the object */
rt::vector object::sample(randomgen& /*rg*/) const {
    return rt::vector();
}

/* Uniformly samples a point on the object that is visible from pt */
rt::vector object::sample_visible(randomgen& /*rg*/, const rt::vector& /*pt*/) const {
    return rt::vector();
}
