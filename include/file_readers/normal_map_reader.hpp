#pragma once

#include "light/vector.hpp"
#include "auxiliary/exit_status.hpp"
#include "image/matrix.hpp"
#include "scene/material/normal_map.hpp"

#include <string>
#include <optional>

std::optional<normal_map::vector_matrix> read_normal_map(const std::string& file_name);