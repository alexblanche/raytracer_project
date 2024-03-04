#include <stdlib.h>
#include <chrono>
#include <random>

#include "auxiliary/randomgen.hpp"

randomgen::randomgen() {
    seed = std::chrono::system_clock::now().time_since_epoch().count();
}

void randomgen::update_seed() {
    seed = seed * 337 + 511;
}

/* Returns a random double between 0 and m */
double random_double(randomgen& r, const double m) {
    r.update_seed();
    std::default_random_engine eng (r.seed);
    std::uniform_real_distribution<double> unif(0, m);
    return unif(eng);
}

/* Returns an array of n random doubles between 0 and m */
std::vector<double> random_double_array(randomgen& r, const unsigned int n, const double m) {
    r.update_seed();
    std::default_random_engine eng (r.seed);

    std::uniform_real_distribution<double> unif(0, m);
    
    std::vector<double> rands(n);
    for (unsigned int i = 0; i < n; i++) {
        rands.at(i) = unif(eng);
    }
    return rands;
}