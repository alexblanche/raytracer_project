#include "headers/hit.hpp"
#include "../screen/headers/color.hpp"
#include "headers/vector.hpp"

#include <iostream>

/** The hit class contains the information
 * of a ray hitting a surface: the ray in question,
 * the point of contact, the normal of the surface at
 * this point and the index of the object the surface belongs to.
*/

/* Constructors */

/* Main constructor */
hit::hit(const ray& g, const rt::vector& p, const rt::vector& n, const unsigned int i)
    : gen(g), point(p), normal(n), obj_index(i) {
        //printf("hit created, index = %u\n", i);
    }


/* Default constructor */
hit::hit() {
    gen = ray();
    point = rt::vector();
    normal = rt::vector();
    obj_index = 0;
}

/* Accessors */

ray hit::get_ray() const {
    return gen;
}

rt::vector hit::get_point() const {
    return point;
}

rt::vector hit::get_normal() const {
    return normal;
}

unsigned int hit::get_obj_index() const {
    return obj_index;
}


