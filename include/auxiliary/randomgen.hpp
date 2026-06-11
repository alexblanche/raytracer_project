#pragma once

#include "parameters.hpp"
#include "auxiliary/timer.hpp"

#include <random>

class randomgen {
    private:
        mutable std::default_random_engine engine;
        mutable std::uniform_real_distribution<real> unif_ratio;
        mutable std::uniform_real_distribution<real> unif_angle;
        mutable std::normal_distribution<real> normal_dist;

    public:

        // Factor 4.0_r to improve camera ray generation computation speed
        randomgen(const real std_dev_anti_aliasing = ANTI_ALIASING)
            :   engine(timer_ms::get_time()),
                unif_ratio(0.0_r, 1.0_r),
                unif_angle(0.0_r, 2.0_r * PI),
                normal_dist(0.0_r, (STRATIFIED_ENABLED ? 4.0_r * std_dev_anti_aliasing : std_dev_anti_aliasing)) {}

        /* Returns a random real between 0 and m */
        inline real random_real(real m) const {
            return m * random_ratio();
        }

        /* Returns a random real between 0 and 1 */
        inline real random_ratio() const {
            return unif_ratio(engine);
        }

        /* Returns a random real between 0 and 2 * PI */
        inline real random_angle() const {
            return unif_angle(engine);
        }

        /* Returns two random reals chosen according to a normal distribution
           of mean m and standard deviation std_dev */
        inline std::pair<real, real> random_pair_normal() const {
            return { normal_dist(engine), normal_dist(engine) };
        } 
};
