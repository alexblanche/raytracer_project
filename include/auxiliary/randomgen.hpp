#pragma once

#include <stdlib.h>
#include <vector>
#include <chrono>
#include <random>

#include "screen/color.hpp"

class randomgen {
    private:
        mutable std::default_random_engine engine;
        mutable std::uniform_real_distribution<real> unif_ratio;
        mutable std::uniform_real_distribution<real> unif_angle;
        mutable std::normal_distribution<real> normal_dist;

    public:

        randomgen(const real std_dev_anti_aliasing) {
            const unsigned int seed = std::chrono::system_clock::now().time_since_epoch().count();
            engine = std::default_random_engine(seed);
            unif_ratio = std::uniform_real_distribution<real>(0.0f, 1.0f);
            unif_angle = std::uniform_real_distribution<real>(0.0f, TWOPI);
            normal_dist = std::normal_distribution<real>(0.0f, std_dev_anti_aliasing);
        }

        /* Returns a random real between 0 and m */
        inline real random_real(real m) const {
            std::uniform_real_distribution<real> unif(0, m);
            return unif(engine);
        }

        /* Returns a random real between 0 and 1 */
        inline real random_ratio() const {
            return unif_ratio(engine);
        }

        /* Returns a random real between 0 and 2 * pi */
        inline real random_angle() const {
            return unif_angle(engine);
        }

        /* Returns two random reals chosen according to a normal distribution
           of mean m and standard deviation std_dev */
        inline std::pair<real, real> random_pair_normal() const {
            return std::pair(normal_dist(engine), normal_dist(engine));
        } 
};
