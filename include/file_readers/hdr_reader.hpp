#pragma once

#include "screen/color.hpp"
#include "file_readers/bmp_reader.hpp"
#include "image/matrix.hpp"

#include <string>
#include <vector>
#include <optional>

std::optional<matrix> read_hdr(const std::string& file_name);
