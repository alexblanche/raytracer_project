#pragma once

#include <limits>
#include <numbers>

/***********************************************************************/

/*** real ***/

// Type alias for floating-point numerical values
using real = double;

constexpr real operator ""_r(unsigned long long int x) { return static_cast<real>(x); }
constexpr real operator ""_r(long double x)            { return static_cast<real>(x); }

constexpr real PI = std::numbers::pi_v<real>;

// Maximum real value
constexpr real infinity = std::numeric_limits<real>::max();

// IEEE754 standard for floating-point representation
constexpr bool IEEE754 = std::numeric_limits<real>::is_iec559;

/***********************************************************************/

/*** Objects ***/

// Object types
enum class object_type {
    Triangle, Quad, Sphere, Plane, Box, Cylinder
};

/***********************************************************************/

/*** Global parameters ***/

constexpr unsigned int  MAX_RAYS           = 1000;
constexpr float         ANTI_ALIASING      = 0.3f;
constexpr bool          STRATIFIED_ENABLED = true;

// Shading of polygon meshes
enum class shading {
    SmoothShading, FlatShading
};
constexpr shading SHADING = shading::SmoothShading;

// Parallelism
enum class parallelism {
    Enabled, Disabled
};
constexpr parallelism PARALLELISM = parallelism::Enabled;

/***********************************************************************/

/*** Macro ***/

// Features to be implemented
constexpr bool TODO_BOX_TEXTURING      = true;
constexpr bool TODO_BOX_SAMPLING       = true;

constexpr bool TODO_CYLINDER_TEXTURING = true;
constexpr bool TODO_CYLINDER_SAMPLING  = true;

constexpr bool TODO_PLANE_SAMPLING     = true;

constexpr bool TODO_ROUGHNESS_MAP      = true;
constexpr bool TODO_DISPLACEMENT_MAP   = true;

constexpr bool TODO_NORMAL_MAP_IN_MTL       = true;
constexpr bool TODO_ROUGHNESS_MAP_IN_MTL    = true;
constexpr bool TODO_DISPLACEMENT_MAP_IN_MTL = true;

constexpr bool TODO_ROTATION_OF_MODELS = true;
