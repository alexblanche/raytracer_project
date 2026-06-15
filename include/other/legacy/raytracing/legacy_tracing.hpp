#pragma once

#include "other/legacy/source/legacy_source.hpp"
#include "other/legacy/objects/legacy_object.hpp"

#include <vector>

/* Tracing the ray */

/* Test raycasting function:
  Casts a ray and returns only the color of the surface hit */
// rt::color raycast(const ray& r, const std::vector<const object*>& obj_set);

/* Ray tracing function: computes the hit of the given ray on the closest object,
    then applies all the light from all the sources (blocked by the other objects),
    and returns the resulting color. */
rt::color raytrace(ray& r, const std::vector<const object*>& obj_set, const std::vector<source>& light_set);