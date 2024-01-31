#pragma once

#include <vector>
#include "../../object/object.hpp"
#include "../../light/source.hpp"

class scene {
    public:
        std::vector<object>& obj_set;
        std::vector<light>& light_set;

};