/*** Legacy raytracer (2014) ***/

#include <iostream>
#include <vector>

#include "screen/screen.hpp"

#include "legacy/source.hpp"
#include "legacy/objects/sphere.hpp"
#include "legacy/objects/plane.hpp"

#include "legacy/raytracing/tracing.hpp"

#include "parallel/parallel.h"
#include <mutex>

#include <chrono>

#include <algorithm>
#include <numeric>


// Parallel for-loop macros
#define PARALLEL_FOR_BEGIN(nb_elements) parallel_for(nb_elements, [&](int start, int end){ for(int i = start; i < end; i++)
#define PARALLEL_FOR_END()})

long int get_time () {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
}

/* ********** Render loop ********** */

/* Sequential version */

void render_loop_seq(std::vector<std::vector<rt::color>>& matrix,
    const int width, const int height, const real dist,
    const rt::vector& screen_center, const std::vector<const object*>& obj_set, const std::vector<source>& light_set,
    long int& time) {

    const long int init_time = get_time();
    const rt::vector zero = rt::vector(0.0f, 0.0f, 0.0f);
    
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {

            std::vector<rt::color>& output = matrix[i];

            const rt::vector direct = (rt::vector(i - screen_center.x, j - screen_center.y, dist)).unit();
            ray r = ray(zero, direct);

            output[j] = raytrace(r, obj_set, light_set);
        }
    }

    const long int curr_time = get_time();
    const long int time_elapsed = curr_time - init_time;
    time += time_elapsed;
    // printf("Seq: %ld ms\n", time_elapsed);
    // fflush(stdout);
}


/* Parallel version */

void render_loop_parallel(std::vector<std::vector<rt::color>>& matrix,
    const int width, const int height, const real dist,
    const rt::vector& screen_center, const std::vector<const object*>& obj_set, const std::vector<source>& light_set,
    long int& time) {
    
    const long int init_time = get_time();
    const rt::vector zero = rt::vector(0.0f, 0.0f, 0.0f);

    PARALLEL_FOR_BEGIN(width) {

        std::vector<rt::color>& output = matrix[i];

        for (int j = 0; j < height; j++) {

            const rt::vector direct = (rt::vector(i - screen_center.x, j - screen_center.y, dist)).unit();
            ray r = ray(zero, direct);
            output[j] = raytrace(r, obj_set, light_set);
        }

    } PARALLEL_FOR_END();

    const long int curr_time = get_time();
    const long int time_elapsed = curr_time - init_time;
    time += time_elapsed;
    // printf("Parallel: %ld ms\n", time_elapsed);
    // fflush(stdout);
}


/* ********************************* */
/* ********************************* */

/* ********* MAIN FUNCTION ********* */


int main(int /*argc*/, char **/*argv*/) {

    const rt::color my_red(230, 15, 15);
    const rt::color my_green(15, 230, 15);
    const rt::color my_blue(15, 15, 230);
    const rt::color my_white(230, 230, 230);
    const rt::color my_black(15, 15, 15);
    /* Non-pure colors, to prevent the "black hole" effect when
     a red surface is under a blue spot */

    /* Orientation of the space:
    negative x on the left, positive on the right
    negative y on the top,  positive on the bottom
    negative z behind the camera, positive in front of it
    */

    /* *************************** */
    /* Scene description */

    // Sphere 0
    const sphere sph0(rt::vector(-400,0,1000), 240, rt::color::WHITE);
    // Sphere 1
    const sphere sph1(rt::vector( 400,0,1000), 240, rt::color::WHITE);

    // Plane 0
    const plane pln0(0, -1, 0, rt::vector(0, 240, 0), rt::color::WHITE);
    // Plane 1
    const plane pln1(0, 0, -1, rt::vector(0, 0, 2000), rt::color::WHITE);

    /* Object set */
    /* Storing pointers allow the overridden methods send and intersect (from sphere, plane)
       to be executed instead of the base (object) one */
    const vector<const object*> obj_set {&sph0, &sph1, &pln0, &pln1};


    /* *************************** */

    // Light sources

    const source light0(rt::vector(-500, 0, 600), rt::color::WHITE);
    const source light1(rt::vector(0, 0, 1000),   my_red);
    const source light2(rt::vector(750, 0, 900),  my_blue);

    // Array of the lights in the scene
    const vector<source> light_set {light0, light1, light2};

    /* *************************** */

    // Screen
    const int width = 1920;
    const int height = 1080;
    const real dist = 400; // Distance between the camera and the image

    // The camera is supposed to be on the origin of the space: (0,0,0)
    
    // Vector that will center the 'screen' in the scene
    const rt::vector screen_center(width/2, height/2, 0);

    std::vector<std::vector<rt::color>> matrix(width, std::vector<rt::color>(height));
    
    const rt::screen scr(width, height);
    scr.fast_copy(matrix, width, height, 1);
    scr.update_from_texture();

    /* Benchmark */

    const size_t number_of_renders = 10;

    long int total_time = 0;

    /* Sequential */
    render_loop_seq(matrix, width, height, dist, screen_center, obj_set, light_set, total_time);
    scr.fast_copy(matrix, width, height, 1);
    scr.update_from_texture();
    total_time = 0;
    const long int seq_time_init = get_time();
    for (size_t i = 0; i < number_of_renders; i++) {
        render_loop_seq(matrix, width, height, dist, screen_center, obj_set, light_set, total_time);
        scr.fast_copy(matrix, width, height, 1);
        scr.update_from_texture();
    }
    const long int seq_curr_time = get_time();
    printf("Total Seq time: %ld ms\n", seq_curr_time - seq_time_init);
    printf("Average time: %d ms\n", (int) ((double) total_time / number_of_renders));
    fflush(stdout);

    /* Parallel */
    const long int par_time_init = get_time();
    render_loop_parallel(matrix, width, height, dist, screen_center, obj_set, light_set, total_time);
    scr.fast_copy(matrix, width, height, 1);
    scr.update_from_texture();
    total_time = 0;
    for (size_t i = 0; i < number_of_renders; i++) {
        render_loop_parallel(matrix, width, height, dist, screen_center, obj_set, light_set, total_time);
        scr.fast_copy(matrix, width, height, 1);
        scr.update_from_texture();
    }
    const long int par_curr_time = get_time();
    printf("Total Parallel time: %ld ms\n", par_curr_time - par_time_init);
    printf("Average time: %d ms\n", (int) ((double) total_time / number_of_renders));
    fflush(stdout);
    
    scr.fast_copy(matrix, width, height, 1);
    scr.update_from_texture();

    scr.wait_quit_event();

    return EXIT_SUCCESS;
}