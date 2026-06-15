#pragma once

#include "image/image.hpp"

/// EXPERIMENTAL, TO BE DELETED

/* Glow rendering for bright lights */

void apply_glow(image& image,
    unsigned int number_of_rays, double threshold, double glow_intensity);