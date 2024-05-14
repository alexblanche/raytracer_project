#include <vector>
#include "screen/color.hpp"
#include "legacy/source.hpp"

// #include <algorithm>
// #include <execution>
// #include <numeric>


/* Light application */

/* Returns a vector of colors resulting from the application of each light source on the given hit,
    while taking other objects into account. */

rt::color apply_lights_obj(const hit& h,
    const std::vector<const object*>& obj_set, const std::vector<source>& light_set) {

    rt::color sum(0, 0, 0);

    for (source const& light : light_set) {
        sum = sum + light.apply_obj(h, obj_set);
    }

    // std::vector<rt::color> applied(light_set.size());

    // auto sum_colors = [](rt::color& col1, rt::color& col2){
    //     return col1 + col2;
    // };
    // auto apply = [&h, &obj_set](source& l) { l.apply_obj(h, obj_set); };
    // const rt::color sum =
    //     std::transform_reduce(
    //         std::execution::par,
    //         light_set.begin(), light_set.end(),
    //         apply,
    //         rt::color(0, 0, 0),
    //         sum_colors
    //     );

    return sum.max_out();
}
