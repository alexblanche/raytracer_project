#pragma once

#include "scene/material/material.hpp"
#include "scene/material/texture.hpp"

#include <vector>
#include <string>

/* Association table material_index -> texture_index */
struct mt_assoc {
    std::vector<unsigned int> m_index;
    std::vector<unsigned int> t_index;

    mt_assoc() : m_index({}), t_index({}) {}
    
    /* Push a new pair (material_index, texture_index) */
    void push(const unsigned int mi, const unsigned int ti) {
        m_index.push_back(mi);
        t_index.push_back(ti);
    }

    /* Lookup function: returns a boolean indicating whether a pair was found,
       and if so, writes in the variable texture_index the texture index associated with material_index */
    bool lookup(const unsigned int mi, unsigned int& ti) {
        for (unsigned int i = 0; i < m_index.size(); i++) {
            if (m_index.at(i) == mi) {
                ti = t_index.at(i);
                return true;
            }
        }
        return false;
    }
};


/* Mtl file parser */

/* Defines the new materials from the mtl file file_name,
   places their name in material_names, and places the materials in material_set

    - When a texture is loaded with map_Ka / Kd, it is placed in texture_set (like in the scene parser)
      and the association table is updated with a new pair (m_index, t_index) so that each material created
      with index m_index must have the texture t_index
    - Textures are loaded each time without checking for duplicates, because I assume that it does not happen often
      (otherwise I need a table to remember the already loaded texture's file names)

   Returns true if the operation was successful */

bool parse_mtl_file(const char* file_name,
    std::vector<std::string>& material_names, std::vector<material>& material_set,
    std::vector<std::string>& texture_names, std::vector<texture>& texture_set,
    mt_assoc& assoc);