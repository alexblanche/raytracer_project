#include "legacy/light/legacy_hit.hpp"
#include "light/vector.hpp"
#include "legacy/objects/legacy_object.hpp"

/** The hit class contains the information
 * of a ray hitting a surface: the ray in question,
 * the point of contact, the normal of the surface at
 * this point and the index of the object the surface belongs to.
*/

/* Constructors */

/* Main constructor */
hit::hit(const rt::vector& point, const rt::vector& normal, const object* hit_object)
    : point(point), normal(normal), hit_object(hit_object) {}


/* Default constructor */
hit::hit() {}
