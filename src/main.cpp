#include <iostream>
#include <cstdlib>
#include "screen/screen.hpp"

#include "file_readers/raw_data.hpp"
#include "file_readers/bmp_reader.hpp"
#include "file_readers/scene_parser.hpp"

#include "render/render_loops.hpp"

// Folder creation
#include <sys/stat.h>
#include <bits/stdc++.h>
#include <sys/types.h>
#include <filesystem>


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
    bool multisample = false;
    unsigned int number_of_samples = 0;
    bool gamma_enabled = false;
    float gamma = 1.0f;
    bool reinhardt_enabled = false;

    if (argc == 1 || atoi(argv[1]) == 0) {
        printf("Number of bounces: %u (default)\n", number_of_bounces);
    }
    else {
        number_of_bounces = atoi(argv[1]);
        printf("Number of bounces: %u\n", number_of_bounces);
    }

    if (argc > 2) {

        int index_arg = 2;
        while (index_arg < argc) {

            if (strcmp(argv[index_arg], "-time") == 0) {
                time_enabled = true;
                index_arg++;

                if (index_arg < argc) {
                    if (strcmp(argv[index_arg], "all") == 0) {
                        time_all = true;
                        index_arg++;
                    }
                }
                continue;
            }

            if (strcmp(argv[index_arg], "-rays") == 0) {
                interactive = false;
                index_arg++;

                if (index_arg < argc) {
                    target_number_of_rays = atoi(argv[index_arg]);
                    index_arg++;
                }
                else {
                    printf("Error, -rays option expects 1 argument\n");
                    return EXIT_FAILURE;
                }
                continue;
            }

            if (strcmp(argv[index_arg], "-multisample") == 0) {
                multisample = true;
                index_arg++;

                if (index_arg < argc) {
                    number_of_samples = atoi(argv[index_arg]);
                    index_arg++;
                }
                else {
                    printf("Error, -multisample option expects 1 argument\n");
                    return EXIT_FAILURE;
                }
                continue;
            }

            if (strcmp(argv[index_arg], "-gamma") == 0) {
                gamma_enabled = true;
                index_arg++;

                if (index_arg < argc) {
                    gamma = 1.0 / atof(argv[index_arg]);
                    index_arg++;
                }
                else {
                    printf("Error, -gamma option expects 1 argument\n");
                    return EXIT_FAILURE;
                }

                continue;
            }

            if (strcmp(argv[index_arg], "-reinhardt") == 0) {
                reinhardt_enabled = true;
                index_arg++;
                continue;
            }
        }
    }

    if (gamma_enabled) printf("Gamma correction: %.1f\n", (1.0 / gamma));
    if (reinhardt_enabled) printf("Reinhardt local tone mapping enabled\n");

    /* Checking if the output directory exists */
    struct stat info;
    bool output_dir_exists = stat("../output", &info) == 0 && info.st_mode & S_IFDIR;

    auto create_dir = [&output_dir_exists]() {
        if (not output_dir_exists) {
            output_dir_exists = std::filesystem::create_directories("../output");
        }
    };

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

    std::optional<scene> scene_opt = parse_scene_descriptor("../scene.txt", ANTI_ALIASING);
    
    if (not scene_opt.has_value()) {
        printf("Scene creation failed\n");
        return EXIT_FAILURE;
    }

    scene& scene = scene_opt.value();
    if (scene.gamma != 1.0f) {
        if (gamma_enabled && scene.gamma != gamma) printf("Warning: gamma correction value passed by command-line argument (%f) was overwritten by scene description (%f)\n",
            gamma, scene.gamma);

        gamma_enabled = true;
        gamma = scene.gamma;
        printf("Gamma correction: %.1f\n", (1.0 / gamma));
    }

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
            if (multisample)
                render_loop_parallel_multisample(matrix, scene, number_of_bounces, number_of_samples);
            else
                render_loop_parallel(matrix, scene, number_of_bounces);

            //if (target_number_of_rays <= 10 || i % 10 == 9) {
                printf("\r%u / %u", i+1, target_number_of_rays);
                fflush(stdout);
            //}
            /* Exporting as rtdata every 100 samples */
            if (i % 100 == 99) {
                create_dir();
                export_raw("../output/image.rtdata", i+1, matrix);
            } 
        }

        printf("\r%u / %u", target_number_of_rays, target_number_of_rays);

        create_dir();
        const bool success_bmp = write_bmp("../output/image.bmp", matrix, target_number_of_rays, gamma);
        if (success_bmp) {
            printf(" Saved as output/image.bmp\n");
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
        if (multisample)
            render_loop_parallel_multisample(matrix, scene, number_of_bounces, number_of_samples);
        else
            render_loop_parallel(matrix, scene, number_of_bounces);
    }

    const rt::screen scr(scene.width, scene.height);
    
    if (gamma_enabled) {
        if (reinhardt_enabled)
            scr.fast_copy_reinhardt(matrix, scene.width, scene.height, 1, gamma);
        else
            scr.fast_copy_gamma(matrix, scene.width, scene.height, 1, gamma);
    }
    else
        scr.fast_copy(matrix, scene.width, scene.height, 1);
    
    // 
    scr.update_from_texture();

    printf("\r                                                   ");
    printf("\rNumber of rays per pixel: 1");
    fflush(stdout);

    for (unsigned int number_of_rays = 2; number_of_rays <= MAX_RAYS; number_of_rays++) {

        if (time_enabled) {
            render_loop_parallel_time(matrix, scene, number_of_bounces, time_all);
        }
        else {
            if (multisample)
                render_loop_parallel_multisample(matrix, scene, number_of_bounces, number_of_samples);
            else
                render_loop_parallel(matrix, scene, number_of_bounces);
        }

        printf("\rNumber of rays per pixel: %u", number_of_rays);
        fflush(stdout);

        if (gamma_enabled) {
            if (reinhardt_enabled)
                scr.fast_copy_reinhardt(matrix, scene.width, scene.height, number_of_rays, gamma);
            else
                scr.fast_copy_gamma(matrix, scene.width, scene.height, number_of_rays, gamma);
        }
        else
            scr.fast_copy(matrix, scene.width, scene.height, number_of_rays);

        scr.update_from_texture();

        int key;
        key = scr.poll_keyboard_event();
        switch (key) {
            case 1:
                /* Esc or the window exit "X" clicked */
                printf("\n");
                return EXIT_SUCCESS;
            case 3:
                /* B */
                /* Export as BMP */
                {
                    create_dir();
                    const bool success_bmp = write_bmp("../output/image.bmp", matrix, number_of_rays, gamma);
                    if (success_bmp) {
                        printf(" Saved as output/image.bmp\n");
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
                    create_dir();
                    const bool success_raw = export_raw("../output/image.rtdata", number_of_rays, matrix);
                    if (success_raw) {
                        printf(" Saved as output/image.rtdata\n");
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

    create_dir();
    const bool success = write_bmp("../output/image_final.bmp", matrix, MAX_RAYS, gamma);
    if (success) {
        printf("\nSaved as output/image_final.bmp\n");
        return EXIT_SUCCESS;
    }
    else {
        printf("\nSave failed\n");
        return EXIT_FAILURE;
    }
}