#pragma once

#include "scene/scene.hpp"

#include <optional>

std::optional<scene> parse_scene_descriptor(const char* file_name);