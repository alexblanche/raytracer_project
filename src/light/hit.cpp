#include "light/hit.hpp"
#include "scene/objects/object.hpp"

#include <cmath>

/** The hit class contains the information
 * of a ray hitting a surface: the ray in question,
 * the point of contact, the normal of the surface at
 * this point and the index of the object the surface belongs to.
*/

/* Constructors */

/* Main constructor */
hit::hit(ray* generator, const rt::vector& point, const rt::vector& normal, const object* hit_object)
    : generator(generator), point(point), normal(normal), hit_object(hit_object) {

    orientation = ((generator->get_direction() | normal) <= 0.0f) ? orientation_type::Inward : orientation_type::Outward;
}

hit::hit(ray* generator, const rt::vector& point, const rt::vector& normal, const object* hit_object, const orientation_type orientation)
    : generator(generator), point(point), normal(normal), hit_object(hit_object), orientation(orientation) {}

hit::hit() {}


