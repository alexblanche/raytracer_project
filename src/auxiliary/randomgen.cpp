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
double randomgen::random_double(const double& m) {
    update_seed();
    std::default_random_engine eng (seed);
    std::uniform_real_distribution<double> unif(0, m);
    return unif(eng);
}

/* Returns an array of n random doubles between 0 and m */
std::vector<double> randomgen::random_double_array(const unsigned int n, const double& m) {
    update_seed();
    std::default_random_engine eng (seed);

    std::uniform_real_distribution<double> unif(0, m);
    
    std::vector<double> rands(n);
    for (size_t i = 0; i < n; i++) {
        rands[i] = unif(eng);
    }
    return rands;
}

/* Returns a random double chosen according to a normal distribution
   of mean m and standard deviation std_dev */
double randomgen::random_double_normal(const double& m, const double& std_dev) {
    update_seed();
    std::default_random_engine eng (seed);
    std::normal_distribution<double> distribution(m, std_dev);
    return distribution(eng);
}

/* Returns two random doubles chosen according to a normal distribution
   of mean m and standard deviation std_dev */
std::pair<double, double> randomgen::random_pair_normal(const double& m, const double& std_dev) {
    update_seed();
    std::default_random_engine eng (seed);
    std::normal_distribution<double> distribution(m, std_dev);
    return std::pair(distribution(eng), distribution(eng));
}