#include <stdlib.h>
#include <ctime>
#include <random>

#include "headers/random.hpp"

/* Returns a random double between 0 and m */
double random_double(const double m) {
    std::uniform_real_distribution<double> unif(0, m);
    std::default_random_engine re;
    return unif(re);
}

/* Returns an array of n random doubles between 0 and m */
std::vector<double> random_double_array(const unsigned int n, const double m) {
    std::uniform_real_distribution<double> unif(0, m);
    std::default_random_engine re;
    
    std::vector<double> rands(n);
    for (unsigned int i = 0; i < n; i++) {
        rands.at(i) = unif(re);
    }

    return rands;
}