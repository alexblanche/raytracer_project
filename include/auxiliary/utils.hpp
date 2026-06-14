#pragma once

#include "parameters.hpp"

// #include <cmath>

#include <type_traits>
#include <bit>

// Experiments around basing float expressions

inline bool is_zero(real x) {
    // if constexpr (sizeof(real) > sizeof(unsigned long int)) {
    //     return std::bit_cast<unsigned long long int, real>(x) == 0ull;
    // }
    // else {
    //     return std::bit_cast<unsigned long int, real>(x) == 0ul;
    // }
    return x == 0.0_r; //std::fpclassify(x) == FP_ZERO;
}

inline bool is_not_zero(real x) {
    //return not is_zero(x);
    return x != 0.0_r; //std::fpclassify(x) != FP_ZERO;
}

inline bool is_positive(real x) {
    return x >= 0.0_r;
    // return not std::signbit(x);
}

inline bool is_positive_not_zero(real x) {
    return x > 0.0_r; //is_not_zero(x) && is_positive(x);
}

inline bool is_negative(real x) {
    return x <= 0.0_r;
    // return std::signbit(x);
}

inline bool is_negative_not_zero(real x) {
    return x < 0.0_r; //is_not_zero(x) && is_negative(x);
}


///////////////////////////////////////////////////

// Determination of the position of the sign bit of floating-point type at compile-time

template<typename UInt>
requires std::is_integral_v<UInt> && std::is_unsigned_v<UInt>
consteval unsigned int only_set_bit_position(UInt x) {

    if (x == 0)
        return static_cast<unsigned int>(-1);

    unsigned int i = 0;
    while ((x & 1) == 0) {
        x >>= 1;
        i++;
    }
    
    return i;
}

template<typename Float>
requires (std::is_floating_point_v<Float> && sizeof(Float) <= 8)
consteval unsigned int sign_bit_position () {
    
    using UInt = std::conditional_t<sizeof(Float) == 4, uint32_t, uint64_t>;
    static_assert(sizeof(UInt) == sizeof(Float));
    
    constexpr Float pos = 1.0;
    constexpr Float neg = -pos;

    constexpr UInt x = std::bit_cast<UInt, Float>(pos) ^ std::bit_cast<UInt, Float>(neg);
    return only_set_bit_position(x);
}