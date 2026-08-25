#pragma once

#include "scene/scene.hpp"
#include "image/image.hpp"

class debug {

    public:

        static void print_hit_info(const scene& scene, int x, int y);

        static image display_search_depth(const scene& scene);

        static void draw_bounding_boxes(const scene& scene, const unsigned int max_depth);

    private:
        static unsigned int compute_search_depth(const scene& scene, const ray& r);
};

