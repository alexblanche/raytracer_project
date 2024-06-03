#pragma once

#include <stdlib.h>
#include <vector>
#include <chrono>
#include <random>

class randomgen {
    public:
        unsigned int seed;
        
        randomgen();

        void update_seed();
        
        /* Returns a random double between 0 and m */
        double random_double(const double& m);

        /* Returns an array of n random doubles between 0 and m */
        std::vector<double> random_double_array(const unsigned int n, const double& m);

        /* Returns a random double chosen according to a normal distribution
           of mean m and standard deviation std_dev */
        double random_double_normal(const double& m, const double& std_dev);

        /* Returns two random doubles chosen according to a normal distribution
           of mean m and standard deviation std_dev */
        std::pair<double, double> random_pair_normal(const double& m, const double& std_dev);
};
