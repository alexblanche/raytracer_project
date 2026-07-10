#pragma once

#include <limits>
#include <numbers>

// Type alias for floating-point numerical values
using real = double;

constexpr real operator ""_r(unsigned long long int x) { return static_cast<real>(x); }
constexpr real operator ""_r(long double x)            { return static_cast<real>(x); }

constexpr real PI = std::numbers::pi_v<real>;

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

// Parallelism
enum class parallelism {
    Enabled, Disabled
};
constexpr parallelism PARALLELISM = parallelism::Enabled;

// Compiler check
#if (defined(__APPLE__) && defined(__clang__))
    #define APPLE_CLANG 1
#else
    #define APPLE_CLANG 0
#endif