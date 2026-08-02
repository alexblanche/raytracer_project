#pragma once

#include "scene/material/barycentric.hpp"

#include "scene/material/normal_map.hpp"
// To do :
// #include "scene/material/roughness_map.hpp"
// #include "scene/material/displacement_map.hpp"

/* Struct representing UV-coordinates */
struct uvcoord {
    real u, v;
};

/* Base class, derived in each object class */
class mapping_info {

    public:

        struct composition {
            bool has_texture;
            bool has_normal_map;
            // bool has_roughness_map; // To be implemented
            // bool has_displacement_map;
        };

        /* Common index in the scene's sets */
        int index;
        composition comp;

        mapping_info(const int index, const composition& comp)
            : index(index), comp(comp) {}

        mapping_info(mapping_info&&) noexcept        = default;
        mapping_info(const mapping_info&)            = delete;
        mapping_info& operator=(mapping_info&&)      = delete;
        mapping_info& operator=(const mapping_info&) = delete;

        uvcoord get_barycenter(const barycentric_info& bary) const;

        inline bool has_texture_information()      const { return comp.has_texture;          }
        inline bool has_normal_information()       const { return comp.has_normal_map;       }
        // inline bool has_roughness_information()    const { return comp.has_roughness_map;    }
        // inline bool has_displacement_information() const { return comp.has_displacement_map; }
};
