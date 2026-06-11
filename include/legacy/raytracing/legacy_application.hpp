#pragma once

#include "legacy/legacy_source.hpp"
#include "legacy/objects/legacy_object.hpp"

#include <vector>

/* Light application */

/* Returns a vector of colors resulting from the application of each light source on the given hit,
    while taking other objects into account. */
rt::color apply_lights_obj(const hit& h,
    const std::vector<const object*>& obj_set, const std::vector<source>& light_set);
