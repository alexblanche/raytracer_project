#include <iostream>
#include <fstream>
#include <cstdlib>
#include <stdlib.h>
#include <vector>
#include <ctime>

#include "screen/color.hpp"
#include "scene/material/material.hpp"
#include "screen/screen.hpp"
#include "light/ray.hpp"

#include "scene/objects/object.hpp"
#include "scene/objects/bounding.hpp"

#include "parallel/parallel.h"

#ifdef __unix__
#include <mutex>
#else
#include "mingw.mutex.h"
#endif

#include "scene/scene.hpp"
#include "scene/camera.hpp"
#include "tracing/tracing.hpp"

#include "file_readers/raw_data.hpp"
#include "file_readers/bmp_reader.hpp"

#include "postprocess/glow.hpp"

using namespace std;

// Parallel for-loop macros
#define PARALLEL_FOR_BEGIN(nb_elements) parallel_for(nb_elements, [&](int start, int end){ for(int i = start; i < end; ++i)
#define PARALLEL_FOR_END()})

#define MAX_RAYS 1000
#define ANTI_ALIASING 0.15


/* ********** Render loop ********** */

/* Main render loop */
void render_loop_parallel(vector<vector<rt::color>>& matrix,
    scene& scene, const unsigned int number_of_bounces) {
    
    mutex m;

    PARALLEL_FOR_BEGIN(scene.width) {

        for (int j = 0; j < scene.height; j++) {

            ray r = scene.cam.depth_of_field_enabled ?
                  scene.cam.gen_ray_dof(i, j, scene.rg)
                : scene.cam.gen_ray_normal(i, j, ANTI_ALIASING, scene.rg);
            const rt::color pixel_col = pathtrace(r, scene, number_of_bounces);
            
            const rt::color current_color = matrix.at(i).at(j);
            const rt::color new_color = current_color + pixel_col;

            // Updating the color matrix
            m.lock();
            matrix.at(i).at(j) = new_color;
            m.unlock();
        }
        
    } PARALLEL_FOR_END();
}

/* Render loop that handles time measurement
   If time_all is true, all lines produce a time measurement and output the estimated total time.
   It time_all is false, only the total time is output at the end.
 */
void render_loop_parallel_time(vector<vector<rt::color>>& matrix,
    scene& scene, const unsigned int number_of_bounces, const bool time_all) {
    
    mutex m;
    float cpt = 0;
    float x = 100.0 / (((double) scene.width) * ((double) scene.height));
    time_t t_init=time(NULL);

    PARALLEL_FOR_BEGIN(scene.width) {

        for (int j = 0; j < scene.height; j++) {

            ray r = scene.cam.depth_of_field_enabled ?
                  scene.cam.gen_ray_dof(i, j, scene.rg)
                : scene.cam.gen_ray_normal(i, j, ANTI_ALIASING, scene.rg);
            const rt::color pixel_col = pathtrace(r, scene, number_of_bounces);
            
            const rt::color current_color = matrix.at(i).at(j);
            const rt::color new_color = current_color + pixel_col;

            // Updating the color matrix
            m.lock();
            cpt += 1;
            matrix.at(i).at(j) = new_color;
            m.unlock();
        }

        if (time_all) {
            m.lock();
            const long int curr_time = time(NULL);
            const long int elapsed = curr_time - t_init;
            const double estimated_time = ((double) elapsed) * 100.0 / (cpt * x);
            printf("%f / 100, ", cpt * x);
            printf("Time elapsed: %ld seconds, Estimated total time: %d seconds = %d minutes %d seconds\n",
                elapsed, (int) estimated_time, (int) (estimated_time / 60.0), ((int) estimated_time) % 60);
            m.unlock();
        }
        
    } PARALLEL_FOR_END();

    const long int curr_time = time(NULL);
    const long int elapsed = curr_time - t_init;
    if (elapsed < 60) {
        printf("\nTotal rendering time: %ld seconds\n", elapsed);
    }
    else {
        printf("\nTotal rendering time: %ld seconds = %d minutes %d seconds\n",
            elapsed, (int) (((float) elapsed) / 60.0), ((int) elapsed) % 60);
    }
}


/* ********************************* */
/* ********************************* */

/* ********* MAIN FUNCTION ********* */

/** Loop keys:
 * Space/Enter: Continue
 * B key:       Save as image.bmp
 * R key:       Save raw data as image.rtdata
 * Esc:         Exit
 */

int main(int argc, char *argv[]) {

    /* Specification of the parameters through console arguments:
    
       ./main.exe [number of bounces] -time [all]
       or
       ./main.exe [number of bounces] -rays [number of rays]

       The first argument is the maximum number of bounces wanted (2 by default).

       If the string "-time" is provided as a second argument, the time taken for each render
       (one sample per pixel) complete will be measured and displayed.
       In this case, if the third argument is the string "all", then the time taken to render
       each vertical line is displayed, as well as the estimated total time (for longer renders).

       If the string "-rays" is provided as a second argument instead, the third argument
       should be an integer: an image will be generated with the given number of rays per pixel,
       and exported to image.bmp.
     */
    unsigned int number_of_bounces = 2;
    bool time_enabled = false;
    bool time_all = false;
    bool interactive = true;
    unsigned int target_number_of_rays = 0;

    if (argc == 1) {
        printf("Number of bounces: %u (default)\n", number_of_bounces);
    }
    else {
        number_of_bounces = atoi(argv[1]);
        printf("Number of bounces: %u\n", number_of_bounces);
    }

    if (argc > 2) {
        if (strcmp(argv[2], "-time") == 0) {
            time_enabled = true;

            if (argc == 4) {
                if (strcmp(argv[3], "all") == 0) {
                    time_all = true;
                }
            }
        }
        else if (strcmp(argv[2], "-rays") == 0) {
            if (argc == 4) {
                interactive = false;
                target_number_of_rays = atoi(argv[3]);
            }
            else {
                printf("Error, wrong number of arguments\n");
                return EXIT_FAILURE;
            }
        }
    }

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


    /**************************************************************************/

    /* Definition of the matrix in which we will write the image */
    vector<vector<rt::color>> matrix(scene.width, vector<rt::color>(scene.height));


    /* Generating an image of target_number_of_rays rays */

    if (not interactive) {

        printf("Rendering...\n");
        printf("0 / %u", target_number_of_rays);
        fflush(stdout);
        
        for (unsigned int i = 0; i < target_number_of_rays; i++) {
            render_loop_parallel(matrix, scene, number_of_bounces);
            if (target_number_of_rays <= 10 || i % 10 == 9) {
                printf("\r%u / %u", i+1, target_number_of_rays);
                fflush(stdout);
            }
            /* Exporting as rtdata every 100 samples */
            if (i % 100 == 99) {
                export_raw("image.rtdata", i+1, matrix);
            } 
        }

        printf("\r%u / %u", target_number_of_rays, target_number_of_rays);

        /*** Test post-process ***/

        vector<vector<rt::color>> post_processed_image = apply_glow(matrix, target_number_of_rays);

        /************************/
        const bool success_bmp = write_bmp("image.bmp", matrix, target_number_of_rays);
        write_bmp("post.bmp", post_processed_image, 1);
        if (success_bmp) {
            printf(" Saved as image.bmp\n");
        }
        else {
            printf("Save failed\n");
            return EXIT_FAILURE;
        }

        //export_raw("image.rtdata", target_number_of_rays, matrix);
        return EXIT_SUCCESS;
    }


    /* Interactive mode */

    printf("Initialization complete, computing the first ray...");
    fflush(stdout);

    if (time_enabled) {
        render_loop_parallel_time(matrix, scene, number_of_bounces, time_all);
    }
    else {
        render_loop_parallel(matrix, scene, number_of_bounces);
    }
    
    // rt::screen* scr = new rt::screen(scene.width, scene.height);
    // scr->copy(matrix, scene.width, scene.height, 1);
    // scr->update();

    printf("\r                                                   ");
    printf("\rNumber of rays per pixel: 1");
    fflush(stdout);

    for (unsigned int number_of_rays = 2; number_of_rays <= MAX_RAYS; number_of_rays++) {

        if (time_enabled) {
            render_loop_parallel_time(matrix, scene, number_of_bounces, time_all);
        }
        else {
            render_loop_parallel(matrix, scene, number_of_bounces);
        }

        printf("\rNumber of rays per pixel: %u", number_of_rays);
        fflush(stdout);

        if (number_of_rays % 10 == 0) {

            /*
            string file_name = "image00";
            file_name[5] = '0' + (char) ((number_of_rays / 100) % 10);
            file_name[6] = '0' + (char) ((number_of_rays / 10) % 10);
            const bool success = export_raw(file_name.data(), number_of_rays, matrix);
            if (success) {
                printf(" (Saved after %u rays as %s)", number_of_rays, file_name.data());
            }
            else {
                printf("\nExport failed\n");
                return EXIT_FAILURE;
            }
            */
            
            const rt::screen* scr = new rt::screen(scene.width, scene.height);
            scr->copy(matrix, scene.width, scene.height, number_of_rays);
            scr->update();
            int key;
            do {
                key = scr->wait_keyboard_event();
            }
            while (key == 0);
            delete(scr);
            switch (key) {
                case 1:
                    /* Esc or the window exit "X" clicked */
                    printf("\n");
                    return EXIT_SUCCESS;
                case 2:
                    /* Space or Enter */
                    break;
                case 3:
                    /* B */
                    /* Export as BMP */
                    {
                        const bool success_bmp = write_bmp("image.bmp", matrix, number_of_rays);
                        if (success_bmp) {
                            printf(" Saved as image.bmp\n");
                        }
                        else {
                            printf("Save failed\n");
                            return EXIT_FAILURE;
                        }
                        break;
                    }
                case 4:
                    /* R */
                    /* Export raw data */
                    {
                        const bool success_raw = export_raw("image.rtdata", number_of_rays, matrix);
                        if (success_raw) {
                            printf(" Saved as image.rtdata\n");
                        }
                        else {
                            printf("Save failed\n");
                            return EXIT_FAILURE;
                        }
                        break;
                    }
                default:
                    break;
            }
        }
    }

    const bool success = write_bmp("image_final.bmp", matrix, MAX_RAYS);
    if (success) {
        printf("\nSaved as image_final.bmp\n");
        return EXIT_SUCCESS;
    }
    else {
        printf("\nSave failed\n");
        return EXIT_FAILURE;
    }
}