#include "utils.hpp"

#include <iostream>

int main() {
    constexpr unsigned int sdouble = sign_bit_position<double>();
    std::cout << sdouble << std::endl;
    // 63

    constexpr unsigned int sfloat = sign_bit_position<float>();
    std::cout << sfloat << std::endl;
    // 31

    constexpr unsigned int sreal = sign_bit_position<real>();
    std::cout << sreal << std::endl;

    return EXIT_SUCCESS;
}