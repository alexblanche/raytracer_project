#pragma once

#include <limits>

// Type alias for floating-point numerical values
using real = float;

// Maximum real value
constexpr float infinity = std::numeric_limits<float>::max();

constexpr float Pi = 3.14159265358979323846f;

constexpr int width = 1920, height = 1080;

constexpr int half_scr_width  = width  / 2;
constexpr int half_scr_height = height / 2;

constexpr float fov_x = 0.3f;
constexpr float fov_y = 0.15f;
