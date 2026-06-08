#pragma once

#include "parameters.hpp"

// #include <cmath>
// #include <bit>

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