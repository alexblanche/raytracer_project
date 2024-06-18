#include <iostream>
#include <cstdlib>
#include <ctime>

#include "screen/screen.hpp"
#include "tracing/tracing.hpp"

#include "file_readers/raw_data.hpp"
#include "file_readers/bmp_reader.hpp"
#include "file_readers/scene_parser.hpp"

#include "render/render_loops.hpp"

#define MAX_RAYS 1000

#include "scene/objects/polygon.hpp"


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

    if (argc == 1 || atoi(argv[1]) == 0) {
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

    // This screen causes the next one to initialize before crashing
    // const rt::screen scr0(200, 300);
    // scr0.update();
    // scr0.wait_quit_event();

    /* *************************** */
    /* Scene description */
    
    /* Orientation of the space:
       negative x to the left, positive x to the right
       negative y to the top,  positive y to the bottom (/!\)
       negative z toward the camera, positive x forward
    */

    std::optional<scene> scene_opt = parse_scene_descriptor("../scene.txt");
    
    if (not scene_opt.has_value()) {
        printf("Scene creation failed\n");
        return EXIT_FAILURE;
    }

    scene& scene = scene_opt.value();

    printf("Number of objects: %zu\n", scene.object_set.size());


    /**************************************************************************/

    /* Definition of the matrix in which we will write the image */
    std::vector<std::vector<rt::color>> matrix(scene.width, std::vector<rt::color>(scene.height));

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

        const bool success_bmp = write_bmp("image.bmp", matrix, target_number_of_rays);
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
            
            const rt::screen scr(scene.width, scene.height);
            scr.copy(matrix, scene.width, scene.height, number_of_rays);
            scr.update();
            int key;
            do {
                key = scr.wait_keyboard_event();
            }
            while (key == 0);
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