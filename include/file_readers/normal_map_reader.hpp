#pragma once

#include "light/vector.hpp"

bool read_normal_map(const char* file_name, std::vector<std::vector<rt::color>>& data);