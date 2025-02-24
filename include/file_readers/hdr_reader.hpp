#pragma once

#include <vector>
#include "screen/color.hpp"
#include "file_readers/bmp_reader.hpp"
#include <optional>

bool print_hdr_info(const char* file_name);

std::optional<dimensions> read_hdr_size(const char* file_name);

bool read_hdr(const char* file_name, std::vector<std::vector<rt::color>>& data);
