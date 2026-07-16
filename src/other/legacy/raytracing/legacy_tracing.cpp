#include "other/legacy/source/legacy_source.hpp"

#include <numeric>

/* Light application */

/* Returns a vector of colors resulting from the application of each light source on the given hit,
    while taking other objects into account. */

static rt::color apply_lights_obj(const hit& h,
    const std::vector<const object*>& obj_set, const std::vector<source>& light_set) {

    auto sum_colors = [](rt::color const& col1, rt::color const& col2){ return col1 + col2; };
    auto apply = [&h, &obj_set](source const& l) { return l.apply_obj(h, obj_set); };

    const rt::color sum =
        std::transform_reduce(
            light_set.begin(), light_set.end(),
            rt::BLACK,
            sum_colors,
            apply
        );

    return sum.get_capped();
}

/* Ray tracing function: computes the hit of the given ray on the closest object,
    then applies all the light from all the sources (blocked by the other objects),
    and returns the resulting color. */
rt::color raytrace(ray& r, const std::vector<const object*>& obj_set, const std::vector<source>& light_set) {

    real dist_to_closest = infinity;
    std::optional<const object*> closest_obj = std::nullopt;

    for (const object* obj : obj_set) {

        const std::optional<real> d = obj->measure_distance(r);
        if (d.has_value() && d.value() < dist_to_closest) {
            dist_to_closest = d.value();
            closest_obj = obj;
        }
    }

    if (not closest_obj.has_value()) {
        // No object hit
        return rt::BLACK;
    }
    
    const hit h = closest_obj.value()->compute_intersection(r, dist_to_closest);
    return apply_lights_obj(h, obj_set, light_set);
}