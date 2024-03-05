#pragma once

#include <vector>
#include "objects/object.hpp"
#include "screen/color.hpp"
#include "auxiliary/randomgen.hpp"
#include "camera.hpp"

class scene {
    public:
        // Color of the background
        rt::color background;

        // Screen parameters
        int width;
        int height;

        // Camera parameters
        camera cam;

        // Random number generator
        randomgen rg;

        // Triangles are grouped by the given number in the bounding box tree-search method
        unsigned int triangles_per_bounding;

        /* Constructor */

        /* Main constructor */
        scene(const rt::color background,
            const int width, const int height,
            const camera& cam,
            const unsigned int triangles_per_bounding);

        /* Constructor from an external file */
        scene(const char* file_name, bool& creation_successful);
};