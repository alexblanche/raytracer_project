#include "render/render_loops.hpp"
#include "tracing/tracing.hpp"

#include <iostream>
#include <cstdlib>
#include <ctime>

#include "parallel/parallel.h"
#include <mutex>

//#define ANTI_ALIASING 0.3f

// Parallel for-loop macros
#define PARALLEL_FOR_BEGIN(nb_elements) parallel_for(nb_elements, [&](int start, int end){ for(int i = start; i < end; ++i)
#define PARALLEL_FOR_END()})

/* ********** Render loops ********** */

/* Sequential version */
void render_loop_seq(std::vector<std::vector<rt::color>>& matrix,
    scene& scene, const unsigned int number_of_bounces) {

    for (int i = 0; i < scene.width; i++) {
        for (int j = 0; j < scene.height; j++) {

            ray r = scene.cam.depth_of_field_enabled ?
                  scene.cam.gen_ray_dof(i, j, scene.rg)
                : scene.cam.gen_ray_normal(i, j, scene.rg);
            const rt::color pixel_col = pathtrace(r, scene, number_of_bounces);

            // Updating the color matrix
            matrix[i][j] = matrix[i][j] + pixel_col;
        }
    }
}


/* Main render loop */
void render_loop_parallel(std::vector<std::vector<rt::color>>& matrix,
    scene& scene, const unsigned int number_of_bounces) {

    // static unsigned int cpt = 0;

    PARALLEL_FOR_BEGIN(scene.width) {

        std::vector<rt::color>& output = matrix[i];

        for (int j = 0; j < scene.height; j++) {

            ray r = scene.cam.depth_of_field_enabled ?
                  scene.cam.gen_ray_dof(i, j, scene.rg)
                : scene.cam.gen_ray_normal(i, j, scene.rg);

            const rt::color new_col = pathtrace(r, scene, number_of_bounces);
            output[j] = output[j] + new_col;
        }

        // cpt++;
        // printf("%u\n", cpt);
        
    } PARALLEL_FOR_END();
}

/* Render loop that handles time measurement
   If time_all is true, all lines produce a time measurement and output the estimated total time.
   It time_all is false, only the total time is output at the end.
 */
void render_loop_parallel_time(std::vector<std::vector<rt::color>>& matrix,
    scene& scene, const unsigned int number_of_bounces, const bool time_all) {
    
    mutex m;
    float cpt = 0;
    float x = 100.0f / (((float) scene.width) * ((float) scene.height));
    const long int t_init = time(0);

    PARALLEL_FOR_BEGIN(scene.width) {

        for (int j = 0; j < scene.height; j++) {

            ray r = scene.cam.depth_of_field_enabled ?
                  scene.cam.gen_ray_dof(i, j, scene.rg)
                : scene.cam.gen_ray_normal(i, j, scene.rg);

            const rt::color pixel_col = pathtrace(r, scene, number_of_bounces);
            
            const rt::color& current_color = matrix[i][j];
            const rt::color new_color = current_color + pixel_col;

            // Updating the color matrix
            m.lock();
            cpt += 1;
            matrix[i][j] = new_color;
            m.unlock();
        }

        if (time_all) {
            m.lock();
            const long int curr_time = time(0);
            const long int elapsed = curr_time - t_init;
            const float estimated_time = ((float) elapsed) * 100.0f / (cpt * x);
            printf("%f / 100, ", cpt * x);
            printf("Time elapsed: %ld seconds, Estimated total time: %d seconds = %d minutes %d seconds\n",
                elapsed, (int) estimated_time, (int) (estimated_time / 60.0), ((int) estimated_time) % 60);
            m.unlock();
        }
        
    } PARALLEL_FOR_END();

    const long int curr_time = time(0);
    const long int elapsed = curr_time - t_init;
    if (time_all) {
        printf("\nTotal rendering time: ");
    }
    else {
        printf("\nRendering time: ");
    }
    
    if (elapsed < 60) {
        printf("%ld seconds\n", elapsed);
    }
    else {
        printf("%ld seconds = %d minutes %d seconds\n",
            elapsed, (int) (((float) elapsed) / 60.0f), ((int) elapsed) % 60);
    }
}


/* After the first hit, several boucing rays are cast */
void render_loop_parallel_multisample(std::vector<std::vector<rt::color>>& matrix,
    scene& scene, const unsigned int number_of_bounces, const unsigned int number_of_samples) {

    PARALLEL_FOR_BEGIN(scene.width) {

        std::vector<rt::color>& output = matrix[i];

        for (int j = 0; j < scene.height; j++) {

            ray r = scene.cam.depth_of_field_enabled ?
                  scene.cam.gen_ray_dof(i, j, scene.rg)
                : scene.cam.gen_ray_normal(i, j, scene.rg);

            const rt::color new_col = pathtrace_multisample(r, scene, number_of_bounces, number_of_samples);

            output[j] = output[j] + new_col;
        }
        
    } PARALLEL_FOR_END();
}