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

        /* Common index for texture, normal_map (and in the future roughness_map, displacement map) in the sets */
        index_type index = EMPTY_INDEX;

    protected:

        mapping_info(const index_type index)
            : index(index) {}

        mapping_info(mapping_info&&) noexcept        = delete;
        mapping_info(const mapping_info&)            = delete;
        mapping_info& operator=(mapping_info&&)      = delete;
        mapping_info& operator=(const mapping_info&) = delete;
};