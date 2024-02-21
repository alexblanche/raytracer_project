#pragma once

#include <vector>
#include "../objects/headers/object.hpp"
//#include "../sources/headers/source.hpp"
#include "../../screen/headers/color.hpp"
#include "../../auxiliary/headers/randomgen.hpp"

class scene {
    public:
        // Color of the background
        const rt::color background;

        // Screen parameters
        int width;
        int height;
        double distance;
        

        // Camera position
        rt::vector position;
        //const rt::vector direction;
        rt::vector screen_center;

        // Random number generator
        randomgen rg;

        // Triangles are grouped by the given number in the bounding box tree-search method
        unsigned int triangles_per_bounding;

        /* Constructor */

        /* Main constructor */
        scene(const rt::color background,
            const int width,
            const int height,
            const double distance,
            const rt::vector position,
            //const rt::vector direction,
            const rt::vector screen_center,
            const unsigned int triangles_per_bounding);

        /* Constructor from an external file */
        scene(const char* file_name);
};