#pragma once

#include "image/matrix.hpp"
#include "file_readers/error.hpp"

#include <string>
#include <expected>

class hdr {
    public:
        static std::expected<matrix, file_reader::error> read_file(const std::string& file_name);
};