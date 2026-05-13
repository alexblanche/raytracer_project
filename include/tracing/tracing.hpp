#include "light/ray.hpp"
#include "screen/color.hpp"
#include "scene/objects/object.hpp"
#include "scene/scene.hpp"
#include "main_menu/runtime_parameters.hpp"

/* Single ray version:
    Computes the hit of the given ray on the closest object,
    then launches one ray, in a direction and with a color depending on the surface material,
    until it is too dim, or a light-emitting object is hit, or the maximum number of bounces is reached. */
rt::color pathtrace(ray& r, const scene& scene, const randomgen& rg, unsigned int bounce,
    russian_roulette_mode russian_roulette_mode, real init_refr_index = 1.0f);
