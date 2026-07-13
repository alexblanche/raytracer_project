#include "render/render_loops.hpp"
#include "tracing/tracing.hpp"
#include "parallel/parallel.hpp"
#include "auxiliary/timer.hpp"

#include <atomic>
#include <iostream>
#include <thread>

/* ********** Render loops ********** */

/* Sequential version */
void render_loop_seq(image& image, const scene& scene, const unsigned int number_of_bounces, const russian_roulette_mode russian_roulette) {

    const randomgen rg;
    constexpr camera::aa_shift NO_SHIFT = { 0.0_r, 0.0_r };

    const worker worker_(scene, rg, number_of_bounces, russian_roulette);

    for (int j = image.height() - 1; const matrix::row row : image.data) {

        for (int i = 0; rt::color& col : row) {

            const ray r = scene.cam.gen_ray(i, j, rg, 0, NO_SHIFT);
            const rt::color pixel_col = worker_.pathtrace(r);
            col += pixel_col;
            i++;
        }
        j--;
    }

    image.increase_sample_count();
}


static void print_estimated_time(uint64_t time_elapsed, float progress) {
    const float estimated_time = static_cast<float>(time_elapsed) / (progress * 10.0f); // (elapsed / (progress / 100)) * 1000 (in ms)
    printf("%.1f%%, ", progress);
    if (time_elapsed < 1000)
        printf("Time elapsed: %llums, estimated total time: %f seconds\n", time_elapsed, estimated_time);
    else {
        const int estimated_time_int = static_cast<int>(estimated_time);
        printf("Time elapsed: %llu seconds, estimated total time: %d seconds = %d minutes %d seconds\n",
            time_elapsed / 1000,
            estimated_time_int,
            estimated_time_int / 60,
            estimated_time_int % 60
        );
    }
}

static void print_render_time(uint64_t elapsed, bool time_all) {
    if (time_all)
        printf("\nTotal rendering time: ");
    else
        printf("   ");
    
    if (elapsed < 3000) {
        printf("%llums\n", elapsed);
        return;
    }
    
    const unsigned int elapsed_s = elapsed / 1000;
    printf("%u seconds", elapsed_s);
    if (elapsed_s >= 60)
        printf(" = %u minutes %u seconds\n", elapsed_s / 60, elapsed_s % 60);
    printf("\n");
}

/* Main render loop
   If time_mode == Full, all lines produce a time measurement and output the estimated total time.
   If time_mode == Simple, only the total time is output at the end.
 */
template<time_mode time_mode>
void render_loop_parallel(image& image, const scene& scene, const unsigned int number_of_bounces, const russian_roulette_mode russian_roulette) {

    constexpr bool time_enabled = time_mode != time_mode::Disabled;
    constexpr bool time_all = time_mode == time_mode::Full;
    const float x = 100.0f / static_cast<float>(scene.height);

    std::atomic_int cpt = 0;
    timer_ms timer;
    if constexpr (time_enabled) {
        timer.start();
    }

    // Anti-aliasing bias
    static const randomgen rg0;
    const camera::aa_shift shift = camera::generate_shift(rg0);
    
    parallel_for(scene.height, [&, number_of_bounces] (int j_start, int j_end) {

        const randomgen rg;
        const worker worker_(scene, rg, number_of_bounces, russian_roulette);

        for (int j = j_start; j < j_end; j++) {

            const matrix::row row = image.data[image.height() - 1 - j];
            for (int i = 0; rt::color& color : row) {

                const ray init_ray = scene.cam.gen_ray(i, j, rg, image.number_of_samples, shift);
                const rt::color new_color = worker_.pathtrace(init_ray);
                color += new_color;
                i++;
            }
        }

        if constexpr (time_all) {
            timer.step();
            cpt += 1;
            const uint64_t elapsed = timer.elapsed();
            const float progress = cpt * x;
            print_estimated_time(elapsed, progress);
        }

    });

    if constexpr (time_enabled) {
        timer.stop();
        const uint64_t elapsed = timer.elapsed();
        print_render_time(elapsed, time_all);
    }

    image.increase_sample_count();
}

void render_loop(image& image, const scene& scene, const unsigned int number_of_bounces, const russian_roulette_mode russian_roulette) {
    render_loop_parallel<time_mode::Disabled>(image, scene, number_of_bounces, russian_roulette);
}

void render_loop_time(image& image, const scene& scene, const unsigned int number_of_bounces,
    const russian_roulette_mode russian_roulette, const time_mode time_mode) {

    switch (time_mode) {
        case time_mode::Simple:
            render_loop_parallel<time_mode::Simple>(image, scene, number_of_bounces, russian_roulette);
            break;
        case time_mode::Full:
            render_loop_parallel<time_mode::Full>(image, scene, number_of_bounces, russian_roulette);
            break;
        default:
            break;
    }
}

// /* After the first hit, several bouncing rays are cast */
void render_loop_parallel_multisample(image&, const scene&, const unsigned int, const unsigned int) {
    printf("Multisample has been temporarily disabled\n");
    exit(EXIT_FAILURE);
}




/// Experiment


void render_loop_parallel_all_at_once(image& image, const scene& scene, const unsigned int number_of_bounces,
    const russian_roulette_mode russian_roulette, const int target) {

    static const randomgen rg0;
    const camera::aa_shift shift = camera::generate_shift(rg0);

    parallel_for(scene.height, [&, number_of_bounces, russian_roulette, target] (int j_start, int j_end) {

        timer_ms timer;
        timer.start();
        
        const randomgen rg;
        const worker worker_(scene, rg, number_of_bounces, russian_roulette);

        for (int j = j_start; j < j_end; j++) {
            const matrix::row row = image.data[image.height() - 1 - j];

            for (int i = 0; rt::color& color : row) {

                for (int k = 0; k < target; k++) {
                    ray r = scene.cam.gen_ray(i, j, rg, image.number_of_samples + k, shift);
                    const rt::color new_col = worker_.pathtrace(r);
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