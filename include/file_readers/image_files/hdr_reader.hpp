#pragma once

#include "image/matrix.hpp"

#include <string>
#include <optional>

class hdr {
    public:
        static std::optional<matrix> read_file(const std::string& file_name);
};