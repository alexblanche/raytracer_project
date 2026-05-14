#pragma once

#include "light/vector.hpp"
#include "auxiliary/exit_status.hpp"

exit_status read_normal_map(const char* file_name, std::vector<std::vector<rt::vector>>& data);