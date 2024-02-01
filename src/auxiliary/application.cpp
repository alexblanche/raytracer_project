#include <vector>
#include "../screen/headers/color.hpp"
#include "../light/headers/source.hpp"


/* Light application */

/* Returns a vector of colors resulting from the application of each light source on the given hit,
    while NOT taking other objects into account. */
/* The formula for the addition of lights is:
    (r1,g1,b1) + (r2,g2,b2) = (min(r1+r2, 255), min(g1+g2, 255), min(b1+b2, 255)) */
/*
vector<rt::color> apply_lights(const hit& h, const vector<source>& light_set) {

    const unsigned int n = light_set.size();
    vector<rt::color> color_set(n);

    for (unsigned int i = 0 ; i < n ; i++) {
        color_set.at(i) = light_set.at(i).apply(h);
    }

    return color_set;
}
*/

/* Returns a vector of colors resulting from the application of each light source on the given hit,
    while taking other objects into account. */
vector<rt::color> apply_lights_obj(const hit& h,
    const vector<const object*>& obj_set, const vector<source>& light_set) {

    const unsigned int n = light_set.size();
    vector<rt::color> color_set(n);

    for (unsigned int i = 0 ; i < n ; i++) {
        color_set.at(i) = light_set.at(i).apply_obj(h, obj_set);
    }

    return color_set;
}
