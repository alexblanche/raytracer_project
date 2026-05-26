/*** Legacy raytracer (2014) ***/

#include "screen/screen.hpp"

#include "legacy/source.hpp"
#include "legacy/objects/sphere.hpp"
#include "legacy/objects/plane.hpp"

#include "legacy/raytracing/tracing.hpp"

#include "parallel/parallel.hpp"

#include <iostream>
#include <vector>
#include <chrono>


// Parallel for-loop macros

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
    constexpr rt::vector zero;
    
    for (int i = 0; i < width; i++) {
        
        std::vector<rt::color>& output = matrix[i];
        const real direct_x = i - screen_center.x;
        rt::vector direct(direct_x, 0, dist);
        const real norm_xz_sq = direct.normsq();

        for (int j = 0; j < height; j++) {

            direct.y = j - screen_center.y;
            const real norm = std::sqrt(std::fma(direct.y, direct.y, norm_xz_sq));
            ray r(zero, direct / norm);
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
    constexpr rt::vector zero;

    PARALLEL_FOR_BEGIN(width) {

        std::vector<rt::color>& output = matrix[i];
        const real direct_x = i - screen_center.x;
        rt::vector direct(direct_x, 0, dist);
        const real norm_xz_sq = direct.normsq();

        for (int j = 0; j < height; j++) {

            direct.y = j - screen_center.y;
            const real norm = std::sqrt(std::fma(direct.y, direct.y, norm_xz_sq));
            ray r(zero, direct / norm);
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


int main(int, char **) {

    constexpr rt::color my_red(230, 15, 15);
    // const rt::color my_green(15, 230, 15);
    constexpr rt::color my_blue(15, 15, 230);
    // const rt::color my_white(230, 230, 230);
    // const rt::color my_black(15, 15, 15);
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
    const sphere sph0(rt::vector(-400,0,1000), 240, rt::WHITE);
    // Sphere 1
    const sphere sph1(rt::vector( 400,0,1000), 240, rt::WHITE);

    // Plane 0
    const plane pln0(0, -1, 0, rt::vector(0, 240, 0), rt::WHITE);
    // Plane 1
    const plane pln1(0, 0, -1, rt::vector(0, 0, 2000), rt::WHITE);

    /* Object set */
    /* Storing pointers allow the overridden methods send and intersect (from sphere, plane)
       to be executed instead of the base (object) one */
    const vector<const object*> obj_set {&sph0, &sph1, &pln0, &pln1};


    /* *************************** */

    // Light sources

    const source light0(rt::vector(-500, 0, 600), rt::WHITE);
    const source light1(rt::vector(0, 0, 1000),   my_red);
    const source light2(rt::vector(750, 0, 900),  my_blue);

    // Array of the lights in the scene
    const vector<source> light_set {light0, light1, light2};

    /* *************************** */

    // Screen
    constexpr int width  = 1920;
    constexpr int height = 1080;
    constexpr real dist  = 400; // Distance between the camera and the image

    // The camera is supposed to be on the origin of the space: (0,0,0)
    
    // Vector that will center the 'screen' in the scene
    constexpr rt::vector screen_center(width / 2, height / 2, 0);

    std::vector<std::vector<rt::color>> matrix(width, std::vector<rt::color>(height));
    
    const rt::screen scr(matrix, width, height);
    scr.fast_copy(1);
    scr.update_from_texture();

    /* Benchmark */

    constexpr size_t number_of_renders = 10;

    long int total_time = 0;

    /* Sequential */
    render_loop_seq(matrix, width, height, dist, screen_center, obj_set, light_set, total_time);
    scr.fast_copy(1);
    scr.update_from_texture();
    total_time = 0;
    const long int seq_time_init = get_time();
    for (size_t i = 0; i < number_of_renders; i++) {
        render_loop_seq(matrix, width, height, dist, screen_center, obj_set, light_set, total_time);
        scr.fast_copy(1);
        scr.update_from_texture();
    }
    const long int seq_curr_time = get_time();
    printf("Total Seq time: %ld ms\n", seq_curr_time - seq_time_init);
    printf("Average time: %d ms\n", (int) ((double) total_time / number_of_renders));

    /* Parallel */
    const long int par_time_init = get_time();
    render_loop_parallel(matrix, width, height, dist, screen_center, obj_set, light_set, total_time);
    scr.fast_copy(1);
    scr.update_from_texture();
    total_time = 0;
    for (size_t i = 0; i < number_of_renders; i++) {
        render_loop_parallel(matrix, width, height, dist, screen_center, obj_set, light_set, total_time);
        scr.fast_copy(1);
        scr.update_from_texture();
    }
    const long int par_curr_time = get_time();
    printf("Total Parallel time: %ld ms\n", par_curr_time - par_time_init);
    printf("Average time: %d ms\n", (int) ((double) total_time / number_of_renders));
    
    scr.fast_copy(1);
    scr.update_from_texture();

    scr.wait_quit_event();

    return EXIT_SUCCESS;
}