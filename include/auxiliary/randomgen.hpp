#pragma once

#include "parameters.hpp"
#include "auxiliary/timer.hpp"

#include <random>

class randomgen {

    private:
        mutable std::mt19937                         engine;
        mutable std::uniform_real_distribution<real> unif_ratio;
        mutable std::uniform_real_distribution<real> unif_angle;
        mutable std::normal_distribution<real>       normal_dist;

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
        template<unsigned int n>
        requires (n >= 2)
        inline std::array<real, n> random_normal() const {
            std::array<real, n> t;
            for (real& p : t)
                p = random_normal();
            return t;
        }
};


// Simpler class for random ratio (in [0, 1)) generation

template<typename Float>
requires std::is_floating_point_v<Float>
class random_ratio_gen {

    private:
        mutable std::mt19937                          engine;
        mutable std::uniform_real_distribution<Float> unif_ratio;
        mutable std::uniform_int_distribution<>       unif_int;

    public:

        random_ratio_gen(int max_index)
            :   engine(timer_ms::get_time()),
                unif_ratio(0, 1),
                unif_int(0, max_index) {}
        
        random_ratio_gen(uint64_t seed, int max_index)
            :   engine(seed),
                unif_ratio(0, 1),
                unif_int(0, max_index) {}
        
        template<typename T>
        requires std::is_same_v<T, Float> || std::is_same_v<T, int>
        T random() const {
            if constexpr (std::is_same_v<T, Float>)
                return unif_ratio(engine);
            else
                return unif_int(engine);
        }

        Float random(Float m) const {
            return m * random<Float>();
        }
};