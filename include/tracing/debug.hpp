#pragma once

#include "scene/scene.hpp"

#include "image/image.hpp"

void print_hit_info(const scene& scene, int x, int y);

image display_search_depth(const scene& scene);

void draw_bounding_boxes(const scene& scene, const unsigned int max_depth);