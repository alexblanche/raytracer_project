#pragma once

#include "light/vector.hpp"
#include "auxiliary/exit_status.hpp"

#include <string>

exit_status read_normal_map(const std::string& file_name, std::vector<std::vector<rt::vector>>& data);