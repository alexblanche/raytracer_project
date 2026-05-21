#include "render/render_loops.hpp"
#include "tracing/tracing.hpp"
#include "tracing/multisample.hpp"
#include "parallel/parallel.hpp"
#include "auxiliary/timer.hpp"

#include <cstdlib>
#include <atomic>

/* ********** Render loops ********** */

/* Sequential version */
void render_loop_seq(std::vector<std::vector<rt::color>>& matrix,
    const scene& scene, const unsigned int number_of_bounces, const russian_roulette_mode russian_roulette) {

    const randomgen rg;

    for (int i = 0; i < scene.width; i++) {
        std::vector<rt::color>& data_line = matrix[i];
        for (int j = 0; j < scene.height; j++) {

            ray r = scene.cam.gen_ray(i, j, rg, 0);
            const rt::color pixel_col = pathtrace(r, scene, rg, number_of_bounces, russian_roulette);
            data_line[j] += pixel_col;
        }
    }
}


/* Main render loop */
void render_loop_parallel(std::vector<std::vector<rt::color>>& matrix,
    const scene& scene, const unsigned int number_of_bounces, const russian_roulette_mode russian_roulette, const unsigned int iteration) {

    PARALLEL_FOR_BEGIN(scene.width) {
        
        static thread_local const randomgen rg;

        std::vector<rt::color>& data_line = matrix[i];

        for (int j = 0; j < scene.height; j++) {

            ray r = scene.cam.gen_ray(i, j, rg, iteration);
            const rt::color new_col = pathtrace(r, scene, rg, number_of_bounces, russian_roulette);
            data_line[j] += new_col;
        }
        
    } PARALLEL_FOR_END();
}

/* Render loop that handles time measurement
   If time_all is true, all lines produce a time measurement and output the estimated total time.
   It time_all is false, only the total time is output at the end.
 */
void render_loop_parallel_time(std::vector<std::vector<rt::color>>& matrix,
    const scene& scene, const unsigned int number_of_bounces, const time_mode time_mode) {

    const bool time_all = time_mode == time_mode::Full;
    const float x = 100.0f / static_cast<float>(scene.width * scene.height);

    std::atomic_int cpt = 0;
    timer_ms timer;
    timer.start();
    
    PARALLEL_FOR_BEGIN(scene.width) {

        static thread_local const randomgen rg;
        std::vector<rt::color>& data_line = matrix[i];

        for (int j = 0; j < scene.height; j++) {

            ray r = scene.cam.gen_ray(i, j, rg, 0);
            const rt::color pixel_col = pathtrace(r, scene, rg, number_of_bounces, russian_roulette_mode::Disabled);
            data_line[j] += pixel_col;
            cpt++;
        }

        if (time_all) {
            timer.step();
            const uint64_t elapsed = timer.elapsed();
            const float progress = cpt * x;
            const float estimated_time = static_cast<float>(elapsed) / (progress * 10.0f);
            const int estimated_time_int = static_cast<int>(estimated_time);
            printf("%.1f%%, ", progress);
            if (elapsed < 1000)
                printf("Time elapsed: %llums, estimated total time: %f seconds\n", elapsed, estimated_time);
            else
                printf("Time elapsed: %llu seconds, estimated total time: %d seconds = %d minutes %d seconds\n",
                    elapsed / 1000,
                    estimated_time_int,
                    estimated_time_int / 60,
                    estimated_time_int % 60
                );
        }
        
    } PARALLEL_FOR_END();

    timer.stop();
    const uint64_t elapsed = timer.elapsed();
    if (time_all)
        printf("\nTotal rendering time: ");
    else
        printf("   ");
    
    if (elapsed < 3000) {
        printf("%llums\n", elapsed);
        return;
    }
    
    const unsigned int elapsed_s = elapsed / 1000;
    if (elapsed_s < 60)
        printf("%u seconds\n", elapsed_s);
    else
        printf("%u seconds = %u minutes %u seconds\n",
            elapsed_s, elapsed_s / 60, elapsed_s % 60);
}


/* After the first hit, several boucing rays are cast */
void render_loop_parallel_multisample(std::vector<std::vector<rt::color>>& matrix,
    const scene& scene, const unsigned int number_of_bounces, const unsigned int number_of_samples) {

    PARALLEL_FOR_BEGIN(scene.width) {

        static thread_local const randomgen rg;
        std::vector<rt::color>& data_line = matrix[i];

        for (int j = 0; j < scene.height; j++) {

            ray r = scene.cam.gen_ray(i, j, rg, 0);
            const rt::color new_col = pathtrace_multisample(r, scene, rg, number_of_bounces, number_of_samples);
            data_line[j] += new_col;
        }
        
    } PARALLEL_FOR_END();
}