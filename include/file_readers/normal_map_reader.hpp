#pragma once

#include "scene/material/normal_map.hpp"

#include <string>
#include <optional>

std::optional<normal_map::vector_matrix> read_normal_map(const std::string& file_name);