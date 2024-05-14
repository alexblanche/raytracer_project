#include <vector>
#include "screen/color.hpp"
#include "legacy/source.hpp"

#include <algorithm>
#include <numeric>


/* Light application */

/* Returns a vector of colors resulting from the application of each light source on the given hit,
    while taking other objects into account. */

rt::color apply_lights_obj(const hit& h,
    const std::vector<const object*>& obj_set, const std::vector<source>& light_set) {

    auto sum_colors = [](rt::color const& col1, rt::color const& col2){ return col1 + col2; };
    auto apply = [&h, &obj_set](source const& l) { return l.apply_obj(h, obj_set); };
    
    const rt::color sum =
        std::transform_reduce(
            light_set.begin(), light_set.end(),
            rt::color(0, 0, 0),
            sum_colors,
            apply
        );

    return sum.max_out();
}
