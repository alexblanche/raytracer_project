#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <vector>
#include <queue>
#include <ctime>

#include "screen/color.hpp"
#include "scene/material/material.hpp"
#include "screen/screen.hpp"
#include "light/ray.hpp"

#include "scene/objects/object.hpp"
#include "scene/objects/triangle.hpp"
#include "scene/objects/bounding.hpp"

#include "parallel/parallel.h"

#ifdef __unix__
#include <mutex>
#else
#include "mingw.mutex.h"
#endif

#include "scene/scene.hpp"
#include "scene/camera.hpp"
#include "auxiliary/tracing.hpp"

#include "file_readers/raw_data.hpp"
#include "file_readers/bmp_reader.hpp"

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

    if (argc == 1) {
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
    
    printf("Initialization...\n");

    /* *************************** */
    /* Scene description */
    
    /* Orientation of the space:
       negative x to the left, positive x to the right
       negative y to the top,  positive y to the bottom (/!\)
       negative z toward the camera, positive x forward
    */
    
    bool parsing_successful;
    scene scene("../scene.txt", parsing_successful);

    if (not parsing_successful) {
        printf("Scene creation failed\n");
        return EXIT_FAILURE;
    }

    printf("Number of objects: %u\n", (unsigned int) scene.object_set.size());


    /* Definition of the matrix in which we will write the image */
    vector<vector<rt::color>> matrix(scene.width, vector<rt::color>(scene.height));

    printf("Initialization complete, computing the first ray...");
    fflush(stdout);

    render_loop_parallel(matrix, scene, number_of_bounces, time_enabled);
    
    /*
    const rt::screen* scr = new rt::screen(scene.width, scene.height);
    scr->copy(matrix, scene.width, scene.height, 1);
    scr->update();

    printf("\r                                                   ");
    printf("\rNumber of rays per pixel: 1");
    fflush(stdout);

    scr->wait_quit_event();
    delete(scr);
    */

    printf("\r                                                   ");
    printf("\rNumber of rays per pixel: 1");
    fflush(stdout);

    for (unsigned int number_of_rays = 2; number_of_rays <= 1000; number_of_rays++) {

        render_loop_parallel(matrix, scene, number_of_bounces, time_enabled);

        printf("\rNumber of rays per pixel: %u", number_of_rays);
        fflush(stdout);

        if (number_of_rays % 10 == 0) {
            
            string file_name = "image00";
            file_name[5] = '0' + (char) ((number_of_rays / 100) % 10);
            file_name[6] = '0' + (char) ((number_of_rays / 10) % 10);
            const bool success = export_raw(file_name.data(), number_of_rays /*10*/, matrix);
            if (success) {
                printf(" (Saved after %u rays as %s)", number_of_rays, file_name.data());
            }
            else {
                printf("\nExport failed\n");
                return EXIT_FAILURE;
            }

            /* Testing: erasing the content of the matrix after each export */
            /*for (int i = 0; i < scene.width; i++) {
                for (int j = 0; j < scene.height; j++) {
                    matrix.at(i).at(j) = rt::color(0, 0, 0);
                }
            }
            */
            

            /*
            const rt::screen* scr = new rt::screen(scene.width, scene.height);
            scr->copy(matrix, scene.width, scene.height, number_of_rays);
            scr->update();
            const bool stop = scr->wait_quit_event();
            delete(scr);
            if (stop) {
                return EXIT_SUCCESS;
            }
            */
        }
    }

    printf("\n");
    return EXIT_SUCCESS;
}