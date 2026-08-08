#pragma once

#include "scene/material/mapping.hpp"

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

        /* Common index for texture, normal_map (and in the future roughness_map, displacement map) in the sets */
        mapping::index_type index;

    protected:

        mapping_info(const mapping::index_type index)
            : index(index) {}

        mapping_info(mapping_info&&) noexcept        = default;
        mapping_info(const mapping_info&)            = delete;
        mapping_info& operator=(mapping_info&&)      = delete;
        mapping_info& operator=(const mapping_info&) = delete;
};