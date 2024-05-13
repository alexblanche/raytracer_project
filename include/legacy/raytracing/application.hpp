#pragma once

#include <vector>
#include "legacy/light/hit.hpp"
#include "screen/color.hpp"
#include "legacy/source.hpp"
#include "legacy/objects/object.hpp"


/* Light application */

/* Returns a vector of colors resulting from the application of each light source on the given hit,
    while taking other objects into account. */
rt::color apply_lights_obj(const hit& h,
    const std::vector<const object*>& obj_set, const std::vector<source>& light_set);
