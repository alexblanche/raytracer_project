#pragma once

#include <vector>
#include "scene/objects/object.hpp"

/* K-means clustering algorithm */

/* Returns a vector of k vectors of object* representing the k clusters */
std::vector<std::vector<const object*>> k_means(std::vector<const object*>& obj, const unsigned int k);