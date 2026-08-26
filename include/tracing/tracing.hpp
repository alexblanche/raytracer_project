#pragma once

#include "light/ray.hpp"
#include "screen/color.hpp"
#include "scene/scene.hpp"
#include "main_menu/runtime_parameters.hpp"

#include "tracing/direction.hpp"
#include "auxiliary/stack_based_custom_stack.hpp"


class worker {
    public:
        const scene& scene_;
        const randomgen& rg;
        
        unsigned int bounce;
        russian_roulette_mode russian_roulette;
        real init_refr_index;

        mutable stack_based_custom_stack<real, 20> refr_stack;

        worker(const scene& scene, const randomgen& rg,
            unsigned int bounce, russian_roulette_mode russian_roulette, real init_refr_index = 1.0_r)

            : scene_(scene), rg(rg), bounce(bounce), russian_roulette(russian_roulette),
              init_refr_index(init_refr_index) {}

        rt::color pathtrace(const ray& init_ray) const;



    private:
        using enum ray_orientation_type;

        struct accumulators {
            rt::color color_materials = rt::WHITE;
            rt::color emitted_colors  = rt::BLACK;

            [[nodiscard]] inline rt::color combine(const rt::color& color) const {
                return fma(color_materials, color, emitted_colors);
            }

            inline void update_emitted_col(const material& m) {
                emitted_colors = combine(m.get_color() * m.get_emission_intensity());
            }

            inline void update_color_mat(const rt::color& local_color) {
                color_materials *= local_color;
            }
        };
    
        /* Auxiliary function that handles the diffuse reflective case */
        [[nodiscard]] ray diffuse_case(const hit& h, const rt::vector& local_normal) const;

        /* Auxiliary function that handles the specular reflective case */
        // Run-time
        [[nodiscard]] ray specular_reflective_case(const hit& h, const direction::bounce_vectors& bounce_v,
            real smoothness) const;
        
        // Compile-time
        template<ray_orientation_type ray_orientation>
        [[nodiscard]] inline ray specular_reflective_case(const hit& h, const direction::bounce_vectors& bounce_v,
            const real smoothness) const {

            /* Direction according to Lambert's cosine law */
            using enum direction::angle;

            const rt::vector central_dir = direction::central_reflected<ray_orientation>(bounce_v, smoothness);
            return ray(
                /* origin */
                h.biased_point<ray_orientation, Outward>(),

                /* direction */
                (smoothness >= 1.0_r) ?
                      central_dir
                    : (fma(direction::random<Pi>(rg), 1.0_r - smoothness, central_dir)).unit()
            );
        }

        [[nodiscard]] ray refractive_case(const hit& h, real scattering,
            const rt::vector& local_normal, const direction::sin_refracted_output& sin_refr,
            real& refr_index, real next_refr_i) const;

        [[nodiscard]] rt::color background_case(const rt::vector& direction, const accumulators& acc) const;

        [[nodiscard]] rt::color full_intensity_case(const accumulators& acc,
            const hit& h, const material& m) const;
        
        struct bounce_parameters {
            const hit& h;
            const material& m;
            const rt::vector& normal;
            const rt::color& color;
            real smoothness;
            bool double_bounce;
        };

        [[nodiscard]] inline bool specular_bounce(const bounce_parameters& param, const direction::bounce_vectors& bounce_v) const;

        struct path_parameters {
            ray r;
            accumulators acc;
            real refr_index;
        };

        void process_bounce(const bounce_parameters& param, path_parameters& out) const;
};
