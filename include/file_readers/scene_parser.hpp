#pragma once

#include "scene/scene.hpp"

#include <optional>
#include <memory>

std::optional<unique_ptr<scene>> parse_scene_descriptor(const char* file_name);