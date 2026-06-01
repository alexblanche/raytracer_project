#pragma once

#include "screen/color.hpp"
#include "file_readers/bmp_reader.hpp"
#include "image/matrix.hpp"

#include <string>
#include <vector>
#include <optional>

//exit_status print_hdr_info(const char* file_name);

std::optional<dimensions> read_hdr_size(const std::string& file_name);

std::optional<matrix> read_hdr(const std::string& file_name);
