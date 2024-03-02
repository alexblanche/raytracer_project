#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <vector>
#include <queue>
#include <ctime>

#include "src/screen/headers/color.hpp"
#include "src/scene/material/headers/material.hpp"
#include "src/screen/headers/screen.hpp"
#include "src/light/headers/ray.hpp"

#include "src/scene/objects/headers/object.hpp"
#include "src/scene/objects/headers/triangle.hpp"
#include "src/scene/objects/headers/bounding.hpp"

#include "parallel/parallel.h"
#include "mingw.mutex.h"

#include "src/scene/headers/scene.hpp"
#include "src/scene/headers/camera.hpp"
#include "src/auxiliary/headers/tracing.hpp"

/******************* Temporary *******************/
// #include "src/file_readers/headers/bmp_reader.hpp"
/*************************************************/

using namespace std;

// Parallel for-loop macros
#define PARALLEL_FOR_BEGIN(nb_elements) parallel_for(nb_elements, [&](int start, int end){ for(int i = start; i < end; ++i)
#define PARALLEL_FOR_END()})


/* ********** Render loop ********** */

void render_loop_parallel(vector<vector<rt::color>>& matrix,
    scene& scene, const unsigned int number_of_bounces, const bool time_enabled) {
    
    mutex m;
    // float cpt = 0;
    // float x = 100.0 / (((double) scene.width) * ((double) scene.height));
    time_t t_init=time(NULL);

    PARALLEL_FOR_BEGIN(scene.width) {

        for (int j = 0; j < scene.height; j++) {
            
            // const rt::vector& direct = (rt::vector(i, j, scene.distance) - scene.screen_center).unit();
            // ray r = ray(scene.position, direct);
            ray r = scene.cam.gen_ray(i, j);

            const rt::color& pixel_col = pathtrace(r, scene, number_of_bounces);
            
            const rt::color& current_color = matrix.at(i).at(j);
            const rt::color new_color = current_color + pixel_col;
            // cpt += 1;

            // Updating the color matrix
            m.lock();
            matrix.at(i).at(j) = new_color;
            m.unlock();
        }

        /*
        m.lock();
        const long int curr_time = time(NULL);
        const long int elapsed = curr_time - t_init;
        const double estimated_time = ((double) elapsed) * 100.0 / (cpt * x);
        printf("%f / 100, ", cpt * x);
        printf("Time elapsed: %ld seconds, Estimated total time: %d seconds = %d minutes\n",
            elapsed, (int) estimated_time, (int) (estimated_time / 60.0));
        

        // printf("\nObject tested at each bounce:\n");
        // for (unsigned int i = 0; i < 5; i++) {
        //     printf("%u: %u (%f per ray), ", i, ray::obj_comp_cpt.at(i), ((double) ray::obj_comp_cpt.at(i)) / ((double) cpt));
        // }
        // printf("\n");
        m.unlock();
        */
        
    } PARALLEL_FOR_END();

    if (time_enabled) {
        const long int curr_time = time(NULL);
        const long int elapsed = curr_time - t_init;
        if (elapsed < 60) {
            printf("\nTime elapsed: %ld seconds\n", elapsed);
        }
        else {
            printf("\nTime elapsed: %ld seconds = %f minutes\n",
                elapsed, ((float) elapsed) / 60.0);
        }
    }

    // printf("\nObject tested at each bounce:\n");
    // for (unsigned int i = 0; i < 5; i++) {
    //     printf("%u: %u (%f per ray), ", i, ray::obj_comp_cpt.at(i), ((double) ray::obj_comp_cpt.at(i)) / (1366 * 768));
    // }
    // printf("\n\n");
}


/* ********************************* */
/* ********************************* */

/* ********* MAIN FUNCTION ********* */


int main(int argc, char *argv[]) {


    /* Specification of the parameters through console arguments:
    
       ./main.exe [number of bounces] [time]

       The first argument is the maximum number of bounces wanted (2 by default).
       If the string "time" is provided as a second argument, the time taken for each render
       (one ray per pixel) complete will be measured and displayed.
     */
    unsigned int number_of_bounces = 2;
    bool time_enabled = false;

    if (argc == 0) {
        printf("Number of bounces: %u (default)\n", number_of_bounces);
    }
    else {
        number_of_bounces = atoi(argv[1]);
        printf("Number of bounces: %u\n", number_of_bounces);
    }

    if (argc > 2) {
        if (strcmp(argv[2], "time") == 0) {
            time_enabled = true;
        }
    }

    
    // printf("Initialization...");

    /* *************************** */
    /* Scene description */
    
    /* Orientation of the space:
       negative x to the left, positive x to the right
       negative y to the top,  positive y to the bottom (/!\)
       negative z toward the camera, positive x forward
    */
    
    scene scene("scene.txt");

    printf("Number of objects: %lu\n", object::set.size());
    
    

    /* Creation of the triangles */
    // const unsigned int triangles_per_terminal = 80;
    // const unsigned int number_of_triangles = 10 * 512;
    // const unsigned int triangles_per_terminal = 64;
    // const unsigned int number_of_triangles = 4096;
    // const unsigned int triangles_per_terminal = 6400;
    // const unsigned int number_of_triangles = 1638400;
    // const unsigned int number_of_triangles = 70000;

    /* Time test:
        100 tpt, 400 tr -> 7"
        100 tpt, 1600 tr -> 12"
        100 tpt, 3200 tr -> 18"
        100 tpt, 6400 tr -> 30"
        100 tpt, 12800 tr -> 55"
        100 tpt, 102400 tr -> 8'35"
        
        100 tpt,  1638400 tr -> 7142" = 119'
        1600 tpt, 1638400 tr -> 6833" = 114' (Optimized to 6645" = 110' with next_bounding pointer)
        6400 tpt, 1638400 tr -> 6687" = 111' (Optimized to 6557" = 109' with next_bounding pointer)
        no bounding, 1638400 tr -> 129251" = 35.9 hours

        Time (only) divided by 19.3... disappointing (optimized 19.7)

        4096 tr:
        no bounding -> 310" = 5'10"
        4 tpt       -> 34"
        16 tpt      -> 23"
        64 tpt      -> 21" (Optimized to 20" with next_bounding pointer)
        128 tpt     -> 22"
        512 tpt     -> 30"
        Improvement by a factor 14.76 (optimized 15.5)
    */

    /* Test: objects tested at each bounce (5 bounces) over all rays:
       64 tpt, 4096 tr:
       0: 150'052'721, 1: 204'548'204, 2: 132'922'393, 3: 145'187'026, 4: 149'899'062
       = (in objects per ray)
       0: 143, 1: 194, 2: 126, 3: 138, 4: 142 (average: 148.6 =~ 2.32 boxes, corresponding to an amplitude of 145 on the x-axis)
       Approximately as much time as 250 triangles (=~ 4096 / 16) with the object method

       6400 tpt, 1638400 tr:
       (at 2.8% of the first ray per pixel)
       0: 1054370820 (35558.2 per ray), 1: 1721877259 (58069.5 per ray), 2: 1222307253 (41221.7 per ray), 3: 1413194670 (47659.3 per ray), 4: 1497105051 (50489.2 per ray)
       (average = 46600 =~ 7.3 boxes, corresponding to an amplitude of 135 on the x-axis)
       
       Comparison with the object method:
       81920 triangles -> 6959" = 116'15"
       
       (new measures)
       81920 triangles -> 8204" = 136' (at 2.24%)
       80000 triangles -> 7973" = 132' (at 2.3%)
       70000 triangles -> 6631" = 110' (at 1.83%)
       (6400 tpt, 1638400 tr -> 6702" = 111' (at 2.68%))

       New optimization (is_hit_by specialized for standard boxes):
       6400 tpt, 1638400 tr -> 6674" = 111' (at 2.86%)
       Insignificant. (weird)
    */
    
    /*
    const double shift = (2 * 620) / (((double) number_of_triangles) - 1);

    const unsigned int nb_obj = object::set.size();

    for(unsigned int i = 0; i < number_of_triangles; i++) {
        new triangle(
            rt::vector(-620 + 0   + shift * ((double) i), -100, 600),
            rt::vector(-620 + 100 + shift * ((double) i),  100, 500),
            rt::vector(-620 + 80  + shift * ((double) i), -200, 700),
            light_material(rt::color(10, 180, 255), 0));
    }
    
    
    // Automatic bounding boxes definition
    queue<const bounding*> bounding_queue;
    
    // Creation of the terminal nodes and their non-terminal containers
    for(unsigned int i = 0; i < number_of_triangles / triangles_per_terminal; i++) {
        vector<unsigned int> v(triangles_per_terminal);
        for(unsigned int j = 0; j < triangles_per_terminal; j++) {
            v.at(j) = nb_obj + i * triangles_per_terminal + j;
        }
        bounding_queue.push(containing_objects(v));
    }

    // Grouping them by two until there is only one left
    while (bounding_queue.size() != 1) {
        const bounding* bd0 = bounding_queue.front();
        bounding_queue.pop();
        const bounding* bd1 = bounding_queue.front();
        bounding_queue.pop();

        const bounding* bd01 = containing_bounding(*bd0, *bd1);
        bounding_queue.push(bd01);
    }
    
    vector<unsigned int> indices(nb_obj);
    for (unsigned int i = 0; i < nb_obj; i++) {
        indices.at(i) = object::set.at(i)->get_index();
    }
    const bounding c(indices);
    const bounding* bd = bounding_queue.front();
    bounding::set = {bd, &c};
    */
    
    /*
    // Temporary: pushing all objects to the bounding set
    vector<unsigned int> indices(object::set.size());
    for (unsigned int i = 0; i < object::set.size(); i++) {
        indices.at(i) = object::set.at(i)->get_index();
    }
    const bounding c(indices);
    bounding::set = {&c};
    */

    /* ********************************************************** */

    /* Test of bmp parsing */

    /*
    int width, height;
    vector<vector<rt::color>> bmpdata = read_bmp("Mandelbrot_bmp.bmp", width, height);

    const rt::screen* bmpscr = new rt::screen(width, height);
    bmpscr->copy(bmpdata, width, height, 1);
    bmpscr->update();
    bmpscr->wait_quit_event();
    delete(bmpscr);

    return 0;
    */


    /* ********************************************************** */




    /* Definition of the matrix in which we will write the image */
    vector<vector<rt::color>> matrix(scene.width, vector<rt::color>(scene.height));

    printf("\rInitialization complete, computing the first ray...");

    render_loop_parallel(matrix, scene, number_of_bounces, time_enabled);
    
    const rt::screen* scr = new rt::screen(scene.width, scene.height);
    scr->copy(matrix, scene.width, scene.height, 1);
    scr->update();

    printf("\r                                                   ");
    printf("\rNumber of rays per pixel: 1");

    scr->wait_quit_event();
    delete(scr);

    unsigned int number_of_rays = 1;
    bool stop = false;

    while (not stop) {
        number_of_rays ++;

        render_loop_parallel(matrix, scene, number_of_bounces, time_enabled);

        printf("\rNumber of rays per pixel: %u", number_of_rays);
        if (number_of_rays % 10 == 0) {
            const rt::screen* scr = new rt::screen(scene.width, scene.height);
            scr->copy(matrix, scene.width, scene.height, number_of_rays);
            scr->update();
            stop = scr->wait_quit_event();
            delete(scr);
        }
    }

    printf("\n");
    return EXIT_SUCCESS;
}