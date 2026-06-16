#pragma once

#include <limits>

// Type alias for floating-point numerical values
using real = double;

constexpr real operator ""_r(unsigned long long int x) { return static_cast<real>(x); }
constexpr real operator ""_r(long double x)            { return static_cast<real>(x); }

constexpr real PI = 3.14159265358979323846_r;

// Maximum real value
constexpr real infinity = std::numeric_limits<real>::max();

// IEEE754 standard for floating-point representation
constexpr bool IEEE754 = std::numeric_limits<real>::is_iec559;


// Shading of polygon meshes
enum class shading {
    SmoothShading, FlatShading
};
constexpr shading SHADING = shading::SmoothShading;

// Global parameters
constexpr unsigned int MAX_RAYS = 1000;
constexpr float ANTI_ALIASING = 0.3f;
constexpr bool STRATIFIED_ENABLED = true;

// Object types
enum class object_type {
    Triangle, Quad, Sphere, Plane, Box, Cylinder
};