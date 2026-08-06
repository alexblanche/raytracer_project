#pragma once

#include "scene/material/barycentric.hpp"

#include "scene/material/normal_map.hpp"
// To do :
// #include "scene/material/roughness_map.hpp"
// #include "scene/material/displacement_map.hpp"

/* ST-coordinates (in object space) */
struct stcoord {
    real s, t;
};

/* UV-coordinates (in texture space) */
struct uvcoord {
    real u, v;
};

/* Abstract base class, derived in each object class as [object_type]::orientation */
class mapping_info {

    public:

        using index_type = unsigned int;

        struct composition {
            bool has_texture;
            bool has_normal_map;
            // bool has_roughness_map; // To be implemented
            // bool has_displacement_map;
        };

        /* Common index for texture, normal_map (and in the future roughness_map, displacement map) in the sets */
        index_type index;
        composition comp;

        inline bool has_texture_information()      const { return comp.has_texture;          }
        inline bool has_normal_information()       const { return comp.has_normal_map;       }
        // inline bool has_roughness_information()    const { return comp.has_roughness_map;    }
        // inline bool has_displacement_information() const { return comp.has_displacement_map; }

        mapping_info() : index(0), comp(false, false) {}

    protected:

        mapping_info(const index_type index, const composition& comp)
            : index(index), comp(comp) {}

        mapping_info(mapping_info&&) noexcept        = delete;
        mapping_info(const mapping_info&)            = delete;
        mapping_info& operator=(mapping_info&&)      = delete;
        mapping_info& operator=(const mapping_info&) = delete;
};