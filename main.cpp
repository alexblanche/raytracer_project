#include <iostream>
#include <fstream>
#include <cstdlib>
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
#include "tracing/tracing.hpp"

#include "file_readers/raw_data.hpp"
#include "file_readers/bmp_reader.hpp"

using namespace std;

// Parallel for-loop macros
#define PARALLEL_FOR_BEGIN(nb_elements) parallel_for(nb_elements, [&](int start, int end){ for(int i = start; i < end; ++i)
#define PARALLEL_FOR_END()})

#define MAX_RAYS 1000


/* ********** Render loop ********** */

/* Main render loop */
void render_loop_parallel(vector<vector<rt::color>>& matrix,
    scene& scene, const unsigned int number_of_bounces) {
    
    mutex m;

    PARALLEL_FOR_BEGIN(scene.width) {

        for (int j = 0; j < scene.height; j++) {

            ray r = scene.cam.gen_ray(i, j);
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

            ray r = scene.cam.gen_ray(i, j);
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
    /* Speed test: manual definition of bounding boxes for the wooden stool */

    // vector<const object*> stool_polygons;
    // for(unsigned int i = 1; i < scene.object_set.size() - 1; i++) {
    //     stool_polygons.push_back(scene.object_set.at(i));
    // }
    vector<const object*> truss_a_poly;
    for(unsigned int i = 0; i < 8; i++) {
        truss_a_poly.push_back(scene.object_set.at(i));
    }

    vector<const object*> sphere_poly;
    for(unsigned int i = 8; i < 2056; i++) {
        sphere_poly.push_back(scene.object_set.at(i));
    }

    vector<const object*> truss_b_poly;
    for(unsigned int i = 2056; i < 2064; i++) {
        truss_b_poly.push_back(scene.object_set.at(i));
    }

    vector<const object*> leg_poly;
    for(unsigned int i = 2064; i < 2084; i++) {
        leg_poly.push_back(scene.object_set.at(i));
    }

    vector<const object*> support_poly;
    for(unsigned int i = 2084; i < 3716; i++) {
        support_poly.push_back(scene.object_set.at(i));
    }

    vector<const object*> top_poly;
    for(unsigned int i = 3716; i < scene.object_set.size() - 2; i++) {
        top_poly.push_back(scene.object_set.at(i));
    }

    vector<const object*> other_objects = {scene.object_set.at(scene.object_set.size() - 2), scene.object_set.at(scene.object_set.size() - 1)};

    const bounding* truss_a_bd = containing_objects(truss_a_poly);
    const bounding* sphere_bd = containing_objects(sphere_poly);
    const bounding* truss_b_bd = containing_objects(truss_b_poly);
    const bounding* leg_bd = containing_objects(leg_poly);
    const bounding* support_bd = containing_objects(support_poly);
    const bounding* top_bd = containing_objects(top_poly);

    vector<const bounding*> stool_children = {truss_a_bd, sphere_bd, truss_b_bd, leg_bd, support_bd, top_bd};
    const bounding* stool_bd = containing_bounding_any(stool_children);

    const bounding* others_bd = new bounding(other_objects);

    scene.bounding_set = {stool_bd, others_bd};

    /* Wrongful bounding box test */
    // const box* stool_b = stool_bd->get_b();
    // const box* testb = new box(stool_b->get_position(), rt::vector(1,0,0), rt::vector(0,1,0), 2*stool_b->get_l1(), 2*stool_b->get_l2(), 2*stool_b->get_l3(), 4);
    // scene.object_set.push_back(testb);

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
        }

        const bool success_bmp = write_bmp("image.bmp", matrix, target_number_of_rays);
        if (success_bmp) {
            printf(" Saved as image.bmp\n");
        }
        else {
            printf("Save failed\n");
            return EXIT_FAILURE;
        }
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
                    /* Esc or X clicked */
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