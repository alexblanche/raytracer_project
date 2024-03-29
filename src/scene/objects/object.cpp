#include "scene/objects/object.hpp"
#include "scene/material/material.hpp"

/* Constructors */

object::object()
    : position(rt::vector()), material_index((unsigned int) -1),
      textured(false) {}

object::object(const rt::vector& position, const unsigned int material_index)
    : position(position), material_index(material_index),
      textured(false) {}

object::object(const rt::vector& position, const unsigned int material_index, const bool textured)
    : position(position), material_index(material_index),
      textured(textured) {}



/* Default versions: these four functions are overridden by derived classes */

/* Intersection determination */

double object::measure_distance(const ray& /*r*/) const {
    return 0;
}

hit object::compute_intersection(ray& /*r*/, const double& /*t*/) const {
    return hit();
}

/* Writes the minimum and maximum coordinates of the object on the three axes */
void object::min_max_coord(double& /*min_x*/, double& /*max_x*/,
    double& /*min_y*/, double& /*max_y*/, double& /*min_z*/, double& /*max_z*/) const {

    return;
}
