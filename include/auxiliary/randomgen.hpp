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
                unif_ratio(0, 1),
                unif_angle(0, 2 * PI),
                normal_dist(0, (STRATIFIED_ENABLED ? 4.0_r * std_dev_anti_aliasing : std_dev_anti_aliasing)) {}

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

        /* Returns one random real chosen according to a normal distribution
           of mean m and standard deviation std_dev */
        inline real random_normal() const {
            return normal_dist(engine);
        }

        /* Returns two random reals chosen according to a normal distribution
           of mean m and standard deviation std_dev */
        inline std::pair<real, real> random_pair_normal() const {
            return { normal_dist(engine), normal_dist(engine) };
        } 
};


// Simpler class for random ratio (in [0, 1)) generation

template<typename Float>
requires std::is_floating_point_v<Float>
class random_ratio_gen {

    private:
        mutable std::default_random_engine engine;
        mutable std::uniform_real_distribution<Float> unif_ratio;

    public:

        random_ratio_gen()
            :   engine(timer_ms::get_time()),
                unif_ratio(0, 1) {}
                
        random_ratio_gen(uint64_t seed)
            :   engine(seed),
                unif_ratio(0, 1) {}

        inline Float random() const {
            return unif_ratio(engine);
        }

        inline Float random(Float m) const {
            return m * random();
        }
};