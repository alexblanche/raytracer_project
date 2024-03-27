#pragma once

#include "scene/material/material.hpp"

#include <vector>
#include <string>

/* Mtl file parser */

/* Defines the new materials from the mtl file file_name,
   places their name in material_names, and places the materials in material_set
   Returns true if the operation was successful */
bool parse_mtl_file(const char* file_name,
    std::vector<std::string>& material_names, std::vector<material>& material_set);