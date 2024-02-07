#include "../../light/headers/ray.hpp"
#include "../../screen/headers/color.hpp"
#include "../../scene/sources/headers/source.hpp"
#include "../../scene/objects/headers/object.hpp"
#include "../../scene/headers/scene.hpp"

#include "application.hpp"

/* Tracing the ray */

/* Test raycasting function:
  Returns only the color of the surface hit by the ray */
rt::color raycast(const ray& r, const vector<const object*>& obj_set);

/* Ray tracing function: computes the hit of the given ray on the closest object,
  then applies all the light from all the sources (blocked by the other objects),
  and returns the resulting color. */
rt::color raytrace(const ray& r, const vector<const object*>& obj_set, const vector<source>& light_set);

/* Path tracing function: computes the hit of the given ray on the closest object,
    then recursively launches rays, with a distribution depending on the surface material,
    until either a light-emitting object is hit, or the maximum number of bounced is reached.
    The colors obtained are then combined to determine the color of the pixel. */
rt::color pathtrace(const ray& r, scene& scene, const unsigned int origin_obj_index,
    const unsigned int number_of_rays, const unsigned int bounce);