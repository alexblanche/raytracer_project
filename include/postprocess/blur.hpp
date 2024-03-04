#pragma once

#include "screen/color.hpp"

/* Blurring algorithm */

/* Computes a box blur of the image, with box of side range (range should be an odd number) */
std::vector<std::vector<rt::color>> blur (const std::vector<std::vector<rt::color>>& image,
    const unsigned int range);