#pragma once

#include "parameters.hpp"
#include "file_readers/parsers/parsing_wrappers.hpp"

namespace mapping {

    using index_type = unsigned int;

    class composition {
        public:
            bool has_texture          = false;
            bool has_normal_map       = false;
            // bool has_roughness_map    = false; // To be implemented
            // bool has_displacement_map = false;

            static_assert(TODO_ROUGHNESS_MAP);
            static_assert(TODO_DISPLACEMENT_MAP);
    };
}

template<>
inline constexpr std::string type_str<mapping::composition>() { return "composition"; }