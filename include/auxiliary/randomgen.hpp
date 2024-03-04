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
};

/* Returns a random double between 0 and m */
double random_double(randomgen& r, const double m);

/* Returns an array of n random doubles between 0 and m */
std::vector<double> random_double_array(randomgen& r, const unsigned int n, const double m);
