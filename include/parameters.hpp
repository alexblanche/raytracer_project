#pragma once

#include <limits>

// Type alias for floating-point numerical values
using real = double;

// Maximum real value
constexpr real infinity = std::numeric_limits<real>::max();
constexpr real PI = 3.14159265358979323846;

// Comment for flat-shading of polygon meshes
enum class shading {
    SmoothShading, FlatShading
};
constexpr shading SHADING = shading::SmoothShading;

constexpr unsigned int MAX_RAYS = 1000;
constexpr float ANTI_ALIASING = 0.3f;

// object types
enum class object_type {
    Triangle, Quad, Sphere, Plane, Box, Cylinder
};