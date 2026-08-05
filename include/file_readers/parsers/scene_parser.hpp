#pragma once

#include "scene/scene.hpp"
#include "file_readers/parsers/parsing_wrappers.hpp"

#include <optional>

struct containers {
    std::vector<const object*>&       object_set;
    std::vector<const object*>&       other_content;
    scene::containers::object&        object_containers;
    std::vector<wrapper<material>>&   material_wrapper_set;
    std::vector<wrapper<texture>>&    texture_wrapper_set;
    std::vector<wrapper<normal_map>>& normal_map_wrapper_set;
    scene::containers::orientation&   orientation_containers;
};

std::optional<scene> parse_scene_descriptor(const std::string& file_name);