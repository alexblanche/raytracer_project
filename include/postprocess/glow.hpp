#pragma once

#include <vector>
#include "screen/color.hpp"

/* Glow rendering for bright lights */

std::vector<std::vector<rt::color>> apply_glow(const std::vector<std::vector<rt::color>>& image,
    const unsigned int number_of_rays, const double& threshold, const double& glow_intensity);