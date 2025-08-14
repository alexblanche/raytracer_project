#include <stdlib.h>
#include <chrono>
#include <random>

#include "auxiliary/randomgen.hpp"

#define TWOPI 6.2831853071795862f

randomgen::randomgen(const real std_dev_anti_aliasing) {
    const unsigned int seed = std::chrono::system_clock::now().time_since_epoch().count();
    engine = std::default_random_engine(seed);
    unif_ratio = std::uniform_real_distribution<real>(0.0f, 1.0f);
    unif_angle = std::uniform_real_distribution<real>(0.0f, TWOPI);
    normal_dist = std::normal_distribution<real>(0.0f, std_dev_anti_aliasing);
}

// void randomgen::update_seed() {
//     seed = seed * 337 + 511;
// }

/* Returns a random real between 0 and m */
real randomgen::random_real(const real m) {
    /*
    update_seed();
    std::default_random_engine eng (seed);
    std::uniform_real_distribution<real> unif(0, m);
    return unif(eng);
    */
     
    std::uniform_real_distribution<real> unif(0, m);
    return unif(engine);
}


/* Returns an array of n random reals between 0 and m */
/*
std::vector<real> randomgen::random_real_array(const size_t n, const real& m) {
    update_seed();
    std::default_random_engine eng (seed);

    std::uniform_real_distribution<real> unif(0, m);
    
    std::vector<real> rands(n);
    for (size_t i = 0; i < n; i++) {
        rands[i] = unif(eng);
    }
    return rands;
}
*/

/* Returns a random real chosen according to a normal distribution
   of mean m and standard deviation std_dev */
/*
real randomgen::random_real_normal(const real& m, const real& std_dev) {
    update_seed();
    std::default_random_engine eng (seed);
    std::normal_distribution<real> distribution(m, std_dev);
    return distribution(eng);
}
*/

/* Returns two random reals chosen according to a normal distribution
   of mean m and standard deviation std_dev */
// std::pair<real, real> randomgen::random_pair_normal(const real& m, const real& std_dev) {
//     /*
//     update_seed();
//     std::default_random_engine eng (seed);
//     std::normal_distribution<real> distribution(m, std_dev);
//     return std::pair(distribution(eng), distribution(eng));
//     */
    
//     std::normal_distribution<real> distribution(m, std_dev);
//     return std::pair(distribution(engine), distribution(engine));
// }

// std::pair<real, real> randomgen::random_pair_normal() {
//     std::normal_distribution<real> distribution(0.0f, std_dev);
//     return std::pair(distribution(engine), distribution(engine));
// }