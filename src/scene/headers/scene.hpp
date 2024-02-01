#pragma once

#include <vector>
#include "../../objects/headers/object.hpp"
#include "../sources/headers/source.hpp"


class scene {
    public:
        std::vector<const object*>& obj_set;
        std::vector<source>& light_set;
        // camera...
};