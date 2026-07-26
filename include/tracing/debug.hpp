#pragma once

#include "scene/scene.hpp"

#include "image/image.hpp"

void print_hit_info(const scene& scene, int x, int y);

image display_search_depth(const scene& scene);