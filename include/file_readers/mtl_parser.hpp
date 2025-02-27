#pragma once

#include "scene/material/material.hpp"
#include "scene/material/texture.hpp"

#include "parsing_wrappers.hpp"

#include <vector>
#include <string>

#include <map>


/* Mtl file parser */

/* Defines the new materials from the mtl file file_name,
   places their name in material_names, and places the materials in material_set

    - When a texture is loaded with map_Ka / Kd, it is placed in texture_set (like in the scene parser)
      and the association table is updated with a new pair (m_index, t_index) so that each material created
      with index m_index must have the texture t_index
    - Textures are loaded each time without checking for duplicates, because I assume that it does not happen often
      (otherwise I need a table to remember the already loaded texture's file names)

   Returns true if the operation was successful */

bool parse_mtl_file(const char* file_name, const std::string& path,
    std::vector<wrapper<material>>& material_wrapper_set,
    std::vector<wrapper<texture>>& texture_wrapper_set,
    std::map<size_t, size_t>& mt_assoc, const real gamma);