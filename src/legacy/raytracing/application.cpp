#include <vector>
#include "screen/color.hpp"
#include "legacy/source.hpp"


/* Light application */

/* Returns a vector of colors resulting from the application of each light source on the given hit,
    while taking other objects into account. */
rt::color apply_lights_obj(const hit& h,
    const std::vector<const object*>& obj_set, const std::vector<source>& light_set) {

    rt::color sum(0, 0, 0);

    for (source light : light_set) {
        sum = sum + light.apply_obj(h, obj_set);
    }

    return sum.max_out();
}
