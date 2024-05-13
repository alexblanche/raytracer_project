#include "legacy/light/hit.hpp"
#include "light/vector.hpp"
#include "legacy/objects/object.hpp"
#include <cmath>

#define TWOPI 6.2831853071795862

/** The hit class contains the information
 * of a ray hitting a surface: the ray in question,
 * the point of contact, the normal of the surface at
 * this point and the index of the object the surface belongs to.
*/

/* Constructors */

/* Main constructor */
hit::hit(ray*& generator, const rt::vector& point, const rt::vector& normal, const object*& hit_object)
    : generator(generator), point(point), normal(normal), hit_object(hit_object), is_hit_bool(true) {}


/* Default constructor */
hit::hit() : is_hit_bool(false) {}
