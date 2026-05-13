#pragma once

#include "light/ray.hpp"
#include "screen/color.hpp"
#include "scene/objects/object.hpp"
#include "scene/scene.hpp"

rt::color pathtrace_multisample(ray& r, const scene& scene, const randomgen& rg, unsigned int bounce, unsigned int number_of_samples);