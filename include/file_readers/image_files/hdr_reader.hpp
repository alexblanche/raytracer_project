#pragma once

#include "image/matrix.hpp"

#include <string>
#include <optional>

std::optional<matrix> read_hdr(const std::string& file_name);
