#include "light/ray.hpp"
#include <limits>
std::numeric_limits<real> realtracing;
const real infinity = realtracing.infinity();
#include "screen/color.hpp"
#include "legacy/source.hpp"
#include "legacy/objects/object.hpp"
#include "legacy/raytracing/application.hpp"
#include <cstddef>

#include <algorithm>
// #include <execution>

/* Tracing the ray */

/* Test raycasting function:
  Casts a ray and returns only the color of the surface hit */
/*
rt::color raycast(const ray& r, const std::vector<const object*>& obj_set) {
    
    real closest = infinity;
    int closest_index = -1;

    for (unsigned int i = 0; i < obj_set.size(); i++) {

        const real d = obj_set.at(i)->measure_distance(r);
        //  d is the distance between the origin of the ray and the
        //    intersection point with the object
        if (d < closest) {
            closest = d;
            closest_index = i;
        }
    }

    if (closest_index != -1) {
        return obj_set.at(closest_index)->get_color();
    }
    else {
        return rt::color::BLACK;
    }
}
*/

/* Ray tracing function: computes the hit of the given ray on the closest object,
    then applies all the light from all the sources (blocked by the other objects),
    and returns the resulting color. */
rt::color raytrace(ray& r, const std::vector<const object*>& obj_set, const std::vector<source>& light_set) {

    /* Unneccessarily complicated */
    /*
    std::vector<real> distance_to_object(obj_set.size());
    std::transform(
        // std::execution::seq,
        obj_set.begin(), obj_set.end(),
        distance_to_object.begin(),
        [&r](const object* obj) { return obj->measure_distance(r); }
    );
    
    auto it = std::min_element(
        // std::execution::seq,
        distance_to_object.begin(), distance_to_object.end()
    );

    const unsigned int index_closest = std::distance(std::begin(distance_to_object), it);
    const real dist = *it;
    const object* closest_obj = obj_set[index_closest];
    
    if (closest_obj != NULL) {
        const hit h = closest_obj->compute_intersection(r, dist);
        return apply_lights_obj(h, obj_set, light_set);
    }
    else {
        // No object hit
        return rt::color::BLACK;
    }
    */

    real dist_to_closest = infinity;
    std::optional<const object*> closest_obj = std::nullopt;

    for (const object* obj : obj_set) {

        const std::optional<real> d = obj->measure_distance(r);
        if (d.has_value() && d.value() < dist_to_closest) {
            dist_to_closest = d.value();
            closest_obj = obj;
        }
    }

    if (closest_obj.has_value()) {
        const hit h = closest_obj.value()->compute_intersection(r, dist_to_closest);
        return apply_lights_obj(h, obj_set, light_set);
    }
    else {
        // No object hit
        return rt::color::BLACK;
    }
}