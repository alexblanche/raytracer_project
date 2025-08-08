#pragma once

#include "scene/scene.hpp"

#include <optional>
#include <memory>

std::optional<scene> parse_scene_descriptor(const char* file_name);