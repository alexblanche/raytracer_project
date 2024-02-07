#pragma once

#include <vector>
#include "../objects/headers/object.hpp"
//#include "../sources/headers/source.hpp"
#include "../../screen/headers/color.hpp"
#include "../../auxiliary/headers/randomgen.hpp"

class scene {
    public:
        const std::vector<const object*>& obj_set;
        // std::vector<source>& light_set;
        
        const rt::color background;

        // Screen parameters
        const int width;
        const int height;
        const double distance;

        // Camera position
        const rt::vector position;
        //const rt::vector direction;
        const rt::vector screen_center;

        // Random number generator
        randomgen rg;


        /* Constructor */

        scene(const std::vector<const object*>& obj_set,
            const rt::color background,
            const int width,
            const int height,
            const double distance,
            const rt::vector position,
            //const rt::vector direction,
            const rt::vector screen_center);
};