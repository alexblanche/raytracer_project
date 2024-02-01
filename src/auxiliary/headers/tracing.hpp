#include "../../light/headers/ray.hpp"
#include "../../screen/headers/color.hpp"
#include "../../light/headers/source.hpp"
#include "../../objects/headers/object.hpp"

#include "application.hpp"

/* Tracing the ray */

/* Test raycasting function:
  Returns only the color of the surface hit by the ray */
rt::color raycast(const ray& r, const vector<const object*>& obj_set);

/* Ray tracing function: computes the hit of the given ray on the closest object,
  then applies all the light from all the sources (blocked by the other objects),
  and returns the resulting color. */
rt::color raytrace(const ray& r, const vector<const object*>& obj_set, const vector<source>& light_set);