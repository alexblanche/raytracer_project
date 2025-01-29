#pragma once

#include <stdlib.h>
#include <vector>
#include <chrono>
#include <random>

#include "screen/color.hpp"

class randomgen {
    public:
        //unsigned int seed;
        std::default_random_engine engine;
        real std_dev;
        
        randomgen(const real std_dev_anti_aliasing);

        //void update_seed();
        
        /* Returns a random real between 0 and m */
        real random_real(const real m);

        /* Returns an array of n random reals between 0 and m */
        //std::vector<real> random_real_array(const size_t n, const real m);

        /* Returns a random real chosen according to a normal distribution
           of mean m and standard deviation std_dev */
        //real random_real_normal(const real m, const real std_dev);

        /* Returns two random reals chosen according to a normal distribution
           of mean m and standard deviation std_dev */
        //std::pair<real, real> random_pair_normal(const real m, const real std_dev);
        std::pair<real, real> random_pair_normal();
};
