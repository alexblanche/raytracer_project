#pragma once

#include "auxiliary/randomgen.hpp"
#include "image/matrix.hpp"

constexpr unsigned int LOWRES_DEFAULT_WIDTH  = 854;
constexpr unsigned int LOWRES_DEFAULT_HEIGHT = 480;


struct light_map_sample {
    unsigned int x, y;  // Sampled point
    real p;             // Probability of the sample
};

class alias_table {

    struct sample {
        unsigned int value;
        real p;
    };

    public:
        using Float = float;

        struct alias_bin {
                                    // For bins[i]:
            Float q = 0.0f;         // Probability of choosing i or alias
            Float p = 0.0f;         // Total probability of sampling i
            unsigned int alias = 0;
        };

        std::vector<alias_bin> bins;
        unsigned int map_width;
        unsigned int map_height;
        unsigned int pt_width;
        unsigned int pt_height;
        Float ratio_x;
        Float ratio_y;

        // Constructs an alias table from a probability table
        // The width and height of the original light map and of the low res image are stored inside the alias table
        alias_table(const std::vector<Float>& prob_table,
            unsigned int map_width,
            unsigned int map_height,
            unsigned int pt_width,
            unsigned int pt_height
        );

        alias_table(const matrix& matrix,
            unsigned int pt_width,
            unsigned int pt_height)

            :   alias_table(compute_low_res_table(matrix), matrix.width, matrix.height, pt_width, pt_height) {}

        // Should be called in each thread at initialization
        inline random_ratio_gen<Float> construct_random_generator() const {
            return random_ratio_gen<Float>(bins.size() - 1);
        }

        inline sample sample_table(const random_ratio_gen<Float>& rg) const {

            const unsigned int i = rg.random<int>();
            const auto [ q, p, alias ] = bins[i];
            return (rg.random<Float>() < q) ?
                  sample { i, p }
                : sample { alias, bins[alias].p };
        }

        light_map_sample sample_light_map(const random_ratio_gen<Float>& rg) const;

    private:
        static std::vector<Float> compute_low_res_table(const matrix& matrix);
};


