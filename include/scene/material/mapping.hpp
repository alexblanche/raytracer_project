#pragma once

#include "parameters.hpp"

class mapping {

    public:
        struct composition {
            bool has_texture          = false;
            bool has_normal_map       = false;
            // bool has_roughness_map    = false; // To be implemented
            // bool has_displacement_map = false;

            constexpr composition() {}

            static_assert(TODO_ROUGHNESS_MAP);
            static_assert(TODO_DISPLACEMENT_MAP);
        };
};