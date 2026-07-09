#pragma once

#include "scene/scene.hpp"
#include "file_readers/parsers/parsing_wrappers.hpp"

#include <optional>

struct containers {
    std::vector<const object*>&       object_set;
    std::vector<const object*>&       other_content;
    
    std::vector<triangle>&            triangle_set;
    std::vector<quad>&                quad_set;
    std::vector<sphere>&              sphere_set;
    std::vector<plane>&               plane_set;
    std::vector<box>&                 box_set;
    std::vector<cylinder>&            cylinder_set;

    std::vector<wrapper<material>>&   material_wrapper_set;
    std::vector<wrapper<texture>>&    texture_wrapper_set;
    std::vector<wrapper<normal_map>>& normal_map_wrapper_set;
    std::vector<texture_info>&        texture_info_set;
};

struct pre_parsing_info {
    unsigned int objects    = 0;
    unsigned int triangles  = 0;
    unsigned int quads      = 0;
    unsigned int spheres    = 0;
    unsigned int planes     = 0;
    unsigned int boxes      = 0;
    unsigned int cylinders  = 0;
    unsigned int materials  = 0;
    unsigned int textures   = 0;
};

std::optional<scene> parse_scene_descriptor(const std::string& file_name);