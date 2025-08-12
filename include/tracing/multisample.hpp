#pragma once

#include "light/ray.hpp"
#include "screen/color.hpp"
#include "scene/objects/object.hpp"
#include "scene/scene.hpp"

rt::color pathtrace_multisample(ray& r, scene& scene, randomgen& rg, const unsigned int bounce, const unsigned int number_of_samples);