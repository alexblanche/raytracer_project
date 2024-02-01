#pragma once

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <vector>

#include "hit.hpp"
#include "../../objects/headers/sphere.hpp"
#include "../../objects/headers/plane.hpp"
#include "../../screen/headers/color.hpp"

using namespace std;

/** The source class represents point-shaped light sources,
 * defined by their position and color.
 */

class source {
    protected:
        rt::vector position;
        rt::color color;

    public :
        /* Constructors */
      
        source(const rt::vector& o, const rt::color& c);

        source();
      
        /* Accessors */

        rt::vector get_position() const;
      
        rt::color get_color() const;


        /* Application of light on surfaces */
      
        /* Applies the color of the light source on the given hit,
           while not taking into account the objects of the scene */
        rt::color apply(const hit& h) const;

        /* Applies the color of the light source on the given hit,
           or black if it is blocked by some object of the scene */
        rt::color apply_obj(const hit& h, const vector<const object*>& obj_set) const;
};
