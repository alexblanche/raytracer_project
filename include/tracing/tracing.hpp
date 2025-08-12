#include "light/ray.hpp"
#include "screen/color.hpp"
#include "scene/objects/object.hpp"
#include "scene/scene.hpp"

// #include "auxiliary/application.hpp"

/* Tracing the ray */

/* Obsolete */

/* Test raycasting function:
  Returns only the color of the surface hit by the ray */
// rt::color raycast(const ray& r, const vector<const object*>& obj_set);

/* Ray tracing function: computes the hit of the given ray on the closest object,
  then applies all the light from all the sources (blocked by the other objects),
  and returns the resulting color. */
// rt::color raytrace(const ray& r, const vector<const object*>& obj_set, const vector<source>& light_set);

/* Path tracing function:
    Multiple ray version:
    Computes the hit of the given ray on the closest object,
    then recursively launches (number_of_rays) rays, with a distribution depending on the surface material,
    until either a light-emitting object is hit, or the maximum number of bounced is reached.
    The colors obtained are then combined to determine the color of the pixel. */
// rt::color pathtrace_mult(const ray& r, scene& scene, const unsigned int origin_obj_index,
//     const unsigned int number_of_rays, const unsigned int bounce);

/* Single ray version:
    Computes the hit of the given ray on the closest object,
    then launches one ray, in a direction and with a color depending on the surface material,
    until it is too dim, or a light-emitting object is hit, or the maximum number of bounces is reached. */
rt::color pathtrace(ray& r, scene& scene, randomgen& rg, const unsigned int bounce, const bool russian_roulette, const real init_refr_index = 1.0f);
