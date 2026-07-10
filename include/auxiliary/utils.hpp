#pragma once

#include "parameters.hpp"

#include <type_traits>
#include <bit>
// #include <cmath>

#include <optional>
#include <span>

// Experiments around basing float expressions

inline bool is_zero(real x) {
    
    // using UInt = std::conditional_t<sizeof(real) == 4, uint32_t, uint64_t>;
    // static_assert(sizeof(UInt) == sizeof(real));
    // return *reinterpret_cast<UInt*>(&x) == static_cast<UInt>(0u);
    return x == 0.0_r;
}

inline bool is_not_zero(real x) {
    //return not is_zero(x);
    return x != 0.0_r;
}

inline bool is_positive(real x) {
    return x >= 0.0_r;
    //return not std::signbit(x);
}

inline bool is_positive_not_zero(real x) {
    return x > 0.0_r; //is_not_zero(x) && is_positive(x);
}

inline bool is_negative(real x) {
    return x <= 0.0_r;
    //return std::signbit(x);
}

inline bool is_negative_not_zero(real x) {
    return x < 0.0_r; //is_not_zero(x) && is_negative(x);
}

///////////////////////////////////////////////////

#if (defined(__APPLE__) && defined(__clang__))
    #define APPLE_CLANG 1
#else
    #define APPLE_CLANG 0
#endif

#if APPLE_CLANG

    // Returns the index of x in the given set of values if it is present, otherwise std::nullopt
    template<typename T, std::size_t extent>
    requires (requires (T x, T y) { x == y; })
    std::optional<unsigned int> index_of(const T& x, const std::span<const T, extent> values) {
        for (unsigned int i = 0; const T& y : values) {
            if (x == y)
                return i;
            i++;
        }
        return std::nullopt;
    }

    // Returns true if x belongs to the given values
    template<typename T>
    requires (requires (T x, T y) { x == y; })
    bool belongs_to(const T& x, const std::span<const T> values) {
        return index_of(x, values).has_value();
    }

#else

    #include <array>

    // Returns the index of x in the given set of values if it is present, otherwise std::nullopt
    template<typename T, std::size_t extent>
    requires (requires (T x, T y) { x == y; })
    std::optional<unsigned int> index_of(const T& x, const std::array<T, extent>& values) {
        for (unsigned int i = 0; const T& y : values) {
            if (x == y)
                return i;
            i++;
        }
        return std::nullopt;
    }

    template<typename T, std::size_t extent>
    requires (requires (T x, T y) { x == y; })
    std::optional<unsigned int> index_of(const T& x, std::array<T, extent>&& values) {
        for (unsigned int i = 0; const T& y : values) {
            if (x == y)
                return i;
            i++;
        }
        return std::nullopt;
    }

    // Returns true if x belongs to the given values
    template<typename T, std::size_t extent>
    requires (requires (T x, T y) { x == y; })
    bool belongs_to(const T& x, std::array<T, extent>&& values) {
        return index_of(x, std::move(values)).has_value();
    }
#endif

///////////////////////////////////////////////////

// Unsigned int type with given size
template<unsigned int UINT_SIZE>
requires (UINT_SIZE >= 2 && UINT_SIZE <= sizeof(real))
using UInt =
    std::conditional_t<UINT_SIZE == 8, uint64_t,
    std::conditional_t<UINT_SIZE == 4, uint32_t, uint16_t>>;

inline bool is_between_zero_and_one(const real x) {

    if constexpr (IEEE754) {
        
        // Extraction of the 12 most significant bits b11 ... b0 (b11 = MSB) of the binary representation of x
        // by splitting it into NB_BLOCKS unsigned integers of type UINT and accessing the last block.
        // b11 is the sign bit of x
        // b10 ... b0 is the exponent part of x
        // In the IEEE754 standard, the actual exponent of x is (1023 - (b10 ... b0))
        // We check whether b11 = 0 && (b10 ... b0) < 1023.
        
        constexpr unsigned int UINT_SIZE = 4; // size of the chosen uint type in bytes

        using UInt = UInt<UINT_SIZE>;
        static_assert(sizeof(UInt) == UINT_SIZE);

        constexpr unsigned int NB_BLOCKS = sizeof(real) / UINT_SIZE;
        constexpr unsigned int UINT_BIT_SIZE = 8 * UINT_SIZE;

        const UInt n = (reinterpret_cast<const UInt*>(&x)[NB_BLOCKS - 1]) >> (UINT_BIT_SIZE - 16);
        
        constexpr UInt max = 1023 << (16 - 12);
        return n < max;
    }
    else {
        return x >= 0.0_r && x <= 1.0_r;
    }
}

// About 1% faster than (std::abs(x) < 1.0_r), consistently. (Disappointing...)
inline bool abs_less_than_one(const real x) {

    if constexpr (IEEE754) {
        constexpr unsigned int UINT_SIZE = 2; // size of the chosen uint type in bytes
        
        using UInt = UInt<UINT_SIZE>;
        static_assert(sizeof(UInt) == UINT_SIZE);

        constexpr unsigned int NB_BLOCKS = sizeof(real) / UINT_SIZE;
        constexpr unsigned int UINT_BIT_SIZE = 8 * UINT_SIZE;
        
        // << 1 to delete the MSB
        const UInt n = ((reinterpret_cast<const UInt*>(&x)[NB_BLOCKS - 1]) << 1) >> (UINT_BIT_SIZE - 16);
        constexpr UInt max = 1023 << (16 - 11);
        return n < max;

        // Alternative: for UINT_SIZE = 8, no right shift
        // const UInt n = (*(reinterpret_cast<const UInt*>(&x)) << 1);
        // constexpr UInt max = 1023ull << (UINT_BIT_SIZE - 11);
        // return n < max;
    }
    else {
        return std::abs(x) < 1.0_r;
    }
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
requires std::is_floating_point_v<Float> && (sizeof(Float) <= 8)
consteval unsigned int sign_bit_position() {

    if constexpr (IEEE754) {
        return 8 * sizeof(Float) - 1;
    }
    else {
        using UInt = UInt<sizeof(Float)>;
        static_assert(sizeof(UInt) == sizeof(Float));
        
        constexpr Float pos = 1.0;
        constexpr Float neg = -pos;

        constexpr UInt diff = std::bit_cast<UInt>(pos) ^ std::bit_cast<UInt>(neg);
        return only_set_bit_position(diff);
    }
}