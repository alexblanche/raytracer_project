#pragma once

#include "parameters.hpp"
#include "scene/material/material.hpp"
#include "math/geometry/vector.hpp"
#include "scene/light_sources/infinite_area.hpp"
#include "math/geometry/trigonometry.hpp"
#include "scene/material/background.hpp"
#include "tracing/direction.hpp"

#include <array>
#include <span>

/* Multiple importance sampling */

namespace sampling {
    enum class strategies {
        CosineWeighted,     // Sample the BSDF
        InfiniteAreaLight   // Sample the background texture
        //LightSample       // Sample the emissive surfaces
        //PortalSample      // Sample in user-defined portals
    };

    constexpr unsigned int number_of_strategies = 2;

    struct direction_sample {
        rt::vector dir;
        real p; // pdf of dir
    };

    namespace strategy {

        class bsdf {
            static direction_sample sample() { return { rt::ZERO, 1 }; }; // Here: overloaded variants, to be used in diffuse_case, etc. in worker

            static real pdf(const rt::vector& dir, const material& m, const direction::bounce_vectors& bounce_v) {
                return 1; // Returns the pdf of the direction dir
            }
        };

        class infinite_area_light {
            static direction_sample sample(
                const alias_table& alt,                         // to be moved in background container
                const random_ratio_gen<alias_table::Float>& rg, // to be moved in worker, alongside its own rg
                const background_container& bg) {               // already in scene

                const auto [ x, y, p ] = alt.sample_light_map(rg);
                const trig::angles angles = {
                    .theta = x * ((2 * PI) / alt.map_width), // Pre-compute both factors
                    .phi   = y * (PI / alt.map_height)
                };

                const rt::vector dir_pre_rotation = trig::direction(angles);
                const rt::vector dir = bg.inverse_rotation * dir_pre_rotation;
                return { dir, p };
            }

            static real pdf(const rt::vector& dir,
                const alias_table& alt,
                const background_container& bg) {

                const rt::vector dir_rotated = bg.rotation_matrix * dir;
                const auto [ theta, phi ] = trig::compute_angles(dir_rotated);

                // Separate the content of infinite_area into 2 classes: alias_table and infinite_area_sampler

                // -> function [ x, y ] = alt.to_low_res_sample(theta, phi);
                const unsigned int x = (theta / (2 * PI)) * alt.pt_width;
                const unsigned int y = (phi / PI) * alt.pt_height;
                
                // -> function p = alt.probability(x, y);
                const unsigned int s = y * alt.pt_width + x;
                const auto [ _, p, _ ] = alt.bins[s];
                return p;
            }
        };
    };

    
};

class monte_carlo_integrator {

    public:
        struct accumulators {
            rt::color incoming_light = rt::WHITE;
            rt::color emitted_light  = rt::BLACK;

            [[nodiscard]] rt::color combine(const rt::color& color) const {
                return fma(incoming_light, color, emitted_light);
            }

            void update_emitted(const rt::color& local_color, const real emission_intensity) {
                emitted_light = combine(local_color * emission_intensity);
            }

            void update_incoming(const rt::color& local_color, const real estimator_weight) {
                incoming_light *= (local_color * estimator_weight);
            }
        };

        enum class type {
            Simple, Multiple
        };
        type type_;
        
        std::array<sampling::strategies, sampling::number_of_strategies>  strats;
        unsigned int nb_strats;

        static void compute_weights(std::array<real, sampling::number_of_strategies>& weights, const material& m) {
            // Assume type_ == Multiple

            /* For now */
            const real s = m.get_smoothness();
            weights[0] = (1.0_r + s) * 0.5_r; // BSDF
            weights[1] = (1.0_r - s) * 0.5_r; // InfiniteArea
        }

        real estimator_weight(const std::span<const real> pdfs, const material& m) const {
            static std::array<real, sampling::number_of_strategies> weights;
            compute_weights(weights, m);

            real p = 0;
            for (int i = 0; i < nb_strats; i++)
                p += weights[i] * pdfs[i];
            return p;
        }

        real estimator(const std::span<const real> pdfs, const material& m, const real cos_theta) const {
            using enum type;

            switch (type_) {

                case Simple: {
                    using enum sampling::strategies;
                    switch (strats[0]) {
                        case CosineWeighted: return 1.0_r;
                        default:             return cos_theta / pdfs[0];
                    }
                    break;
                }

                case Multiple: {
                    return cos_theta / estimator_weight(pdfs, m);
                }

                default: throw;
            }
        }
        
};