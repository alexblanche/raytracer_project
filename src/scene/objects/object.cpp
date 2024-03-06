#include "scene/objects/object.hpp"
#include "scene/material/material.hpp"

/* Constructors */

object::object()
    : position(rt::vector()), index(-1), mat(material()) {}

object::object(const rt::vector& position, const material& m, const unsigned int index)
    : position(position), index(index), mat(m) {}



/* Default versions: these four functions are overridden by derived classes */

/* Intersection determination */

double object::measure_distance(const ray& /*r*/) const {
    return 0;
}

hit object::compute_intersection(const ray& /*r*/, const double /*t*/) const {
    return hit();
}

/* Writes the barycentric coordinates in variables l1, l2
   The boolean return value is used for determining the three points considered in quads */
bool object::get_barycentric(const rt::vector& /*p*/, double& /*l1*/, double& /*l2*/) const {
    return true;
}

/* Writes the minimum and maximum coordinates of the object on the three axes */
void object::min_max_coord(double& /*min_x*/, double& /*max_x*/,
    double& /*min_y*/, double& /*max_y*/, double& /*min_z*/, double& /*max_z*/) const {

    return;
}
