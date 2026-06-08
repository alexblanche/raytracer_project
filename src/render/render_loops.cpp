#include "render/render_loops.hpp"
#include "tracing/tracing.hpp"
#include "tracing/multisample.hpp"
#include "parallel/parallel.hpp"
#include "auxiliary/timer.hpp"

#include <atomic>
#include <iostream>
#include <thread>

/* ********** Render loops ********** */

/* Sequential version */
void render_loop_seq(image& image, const scene& scene, const unsigned int number_of_bounces, const russian_roulette_mode russian_roulette) {

    const randomgen rg;

    for (int j = image.height() - 1; const matrix::row row : image.data) {

        for (int i = 0; rt::color& col : row) {

            const ray r = scene.cam.gen_ray(i, j, rg, 0);
            const rt::color pixel_col = pathtrace(r, scene, rg, number_of_bounces, russian_roulette);
            col += pixel_col;
            i++;
        }
        j--;
    }

    image.increase_sample_count();
}


/* Main render loop */
void render_loop_parallel(image& image, const scene& scene, const unsigned int number_of_bounces, const russian_roulette_mode russian_roulette) {

    parallel_for(scene.height, [&, number_of_bounces, russian_roulette] (int j) {
        
        static thread_local const randomgen rg;

        const matrix::row row = image.data[image.height() - 1 - j];
        for (int i = 0; rt::color& color : row) {

            const ray r = scene.cam.gen_ray(i, j, rg, image.number_of_samples);
            const rt::color new_col = pathtrace(r, scene, rg, number_of_bounces, russian_roulette);
            color += new_col;
            i++;
        }
    });

    image.increase_sample_count();
}

/* Render loop that handles time measurement
   If time_mode == Full, all lines produce a time measurement and output the estimated total time.
   Otherwise, only the total time is output at the end.
 */
void render_loop_parallel_time(image& image, const scene& scene, const unsigned int number_of_bounces, const time_mode time_mode) {

    const bool time_all = time_mode == time_mode::Full;
    const float x = 100.0f / static_cast<float>(scene.width * scene.height);

    std::atomic_int cpt = 0;
    timer_ms timer;
    timer.start();
    
    parallel_for(scene.height, [&, number_of_bounces] (int j) {

        static thread_local const randomgen rg;

        const matrix::row row = image.data[image.height() - 1 - j];
        for (int i = 0; rt::color& col : row) {

            const ray r = scene.cam.gen_ray(i, j, rg, 0);
            const rt::color pixel_col = pathtrace(r, scene, rg, number_of_bounces, russian_roulette_mode::Disabled);
            col += pixel_col;
            i++;
        }

        if (time_all) {
            timer.step();
            cpt += image.width();
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
        
    });

    timer.stop();
    const uint64_t elapsed = timer.elapsed();

    image.increase_sample_count();

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


// /* After the first hit, several bouncing rays are cast */
// void render_loop_parallel_multisample(image& image, const scene& scene, const unsigned int number_of_bounces, const unsigned int number_of_samples) {

//     parallel_for(scene.height, [&, number_of_bounces, number_of_samples] (int j) {

//         static thread_local const randomgen rg;

//         const matrix::row row = image.data[image.height() - 1 - j];
//         for (int i = 0; rt::color& col : row) {

//             const ray r = scene.cam.gen_ray(i, j, rg, 0);
//             const rt::color new_col = pathtrace_multisample(r, scene, rg, number_of_bounces, number_of_samples);
//             col += new_col;
//             i++;
//         }
        
//     });
// }
// Temporary
void render_loop_parallel_multisample(image& image, const scene& scene, const unsigned int number_of_bounces, const unsigned int number_of_samples) {
    if (number_of_samples != 1)
        return;
    return render_loop_parallel(image, scene, number_of_bounces, russian_roulette_mode::Disabled);
}




/// Experiment


void render_loop_parallel_all_at_once(image& image, const scene& scene, const unsigned int number_of_bounces,
    const russian_roulette_mode russian_roulette, const unsigned int target) {

    parallel_for(scene.height, [&, number_of_bounces, russian_roulette, target] (int jstart, int jend) {

        timer_ms timer;
        timer.start();
        
        static thread_local const randomgen rg;

        for (int j = jstart; j < jend; j++) {
            const matrix::row row = image.data[image.height() - 1 - j];

            for (int i = 0; rt::color& color : row) {

                for (unsigned int k = 0; k < target; k++) {
                    ray r = scene.cam.gen_ray(i, j, rg, image.number_of_samples + k);
                    const rt::color new_col = pathtrace(r, scene, rg, number_of_bounces, russian_roulette);
                    color += new_col;
                }
                i++;
            }
        }
    
        timer.stop();
        std::cout << "Thread " << std::this_thread::get_id() << ": ";
        timer.print();
    });

    image.increase_sample_count(target);
}

/*
All at once:
not much improvement, but large disparity between threads:
Thread 0x1fa171e80: Time: 0ms (final: 0 rows)
Thread 0x16b587000: Time: 12965ms
Thread 0x16b613000: Time: 13153ms
Thread 0x16b69f000: Time: 16168ms
Thread 0x16b72b000: Time: 25077ms
Thread 0x16b7b7000: Time: 31219ms
Thread 0x16ba73000: Time: 34838ms
Thread 0x16b843000: Time: 35104ms
Thread 0x16b8cf000: Time: 35434ms
Thread 0x16b95b000: Time: 37609ms
Thread 0x16b9e7000: Time: 38658ms

Room for improvement, by splitting rows by time taken

Total work time:
12965 + 13153 + 16168 + 25077 + 31219 + 34838 + 35104 + 35434 + 37609 + 38658 = 280225 = 280s
Split among 10 threads: 28s
38 / 28 = 1,357
100 * (38-28)/38 = 26,316 -> 26% less time, 1.3x faster at best (on this particular scene: scene_sunny.txt)
*/