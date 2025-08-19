#include <iostream>
#include <cstdlib>
#include "screen/screen.hpp"

#include "file_readers/raw_data.hpp"
#include "file_readers/bmp_reader.hpp"
#include "file_readers/scene_parser.hpp"

#include "render/render_loops.hpp"

// Folder creation
#include <sys/stat.h>
//#include <bits/stdc++.h>
#include <sys/types.h>
#include <filesystem>

#include <ctime>

/* ********* MAIN FUNCTION ********* */

/** Loop keys:
 * Space/Enter: Continue
 * B key:       Save as image.bmp
 * R key:       Save raw data as image.rtdata
 * Esc:         Exit
 */

#include "file_readers/hdr_reader.hpp"
#include "scene/light_sources/infinite_area.hpp"

#include "scene/objects/triangle.hpp"
#include "scene/objects/sphere.hpp"
#include "scene/objects/plane.hpp"

int main(int argc, char *argv[]) {

    // printf("real : %llu, color : %llu, vector : %llu\n", sizeof(real), sizeof(rt::color), sizeof(rt::vector));
    printf("size_t : %llu ; unsigned int : %llu\n", sizeof(size_t), sizeof(unsigned int));

    printf("material : %llu\n", sizeof(material));
    printf("object : %llu\n", sizeof(object));
    printf("texture_info : %llu\n", sizeof(texture_info));
    printf("triangle : %llu\n", sizeof(triangle));
    printf("sphere : %llu\n", sizeof(sphere));
    printf("vector : %llu\n", sizeof(rt::vector));
    printf("plane : %llu\n", sizeof(plane));

    ///// TEMP: testing low res infinite area
    /*
    const char* file_name = "../../../raytracer_project/sky/dome/garden_8k.hdr";
    std::optional<dimensions> size = read_hdr_size(file_name);
    std::vector<std::vector<rt::color>> data(size.value().width, std::vector<rt::color>(size.value().height));
    bool hdr_success = read_hdr(file_name, data);
    if (!hdr_success) throw;
    
    std::vector<real> lrt = compute_low_res_table(data);
    std::vector<std::vector<rt::color>> lrdata(854, std::vector<rt::color>(480));
    // for (unsigned int i = 0; i < LOWRES_DEFAULT_WIDTH; i++) {
    //     for (unsigned int j = 0; j < LOWRES_DEFAULT_HEIGHT; j++) {
    //         const real x = 255.0f * 100000.0f * lrt[j * LOWRES_DEFAULT_WIDTH + i];
    //         //printf("%f\n", x);
    //         lrdata[i][j] = rt::color(x, x, x);
    //     }
    // }

    // const rt::screen test_scr(LOWRES_DEFAULT_WIDTH, LOWRES_DEFAULT_HEIGHT);
    // test_scr.fast_copy(lrdata, LOWRES_DEFAULT_WIDTH, LOWRES_DEFAULT_HEIGHT, 1);
    // test_scr.update_from_texture();
    // test_scr.wait_keyboard_event();
    
    alias_table alt(lrt, size.value().width, size.value().height, LOWRES_DEFAULT_WIDTH, LOWRES_DEFAULT_HEIGHT);
    randomgen rand(ANTI_ALIASING);
    // std::vector<unsigned int> samples(1000);
    // for (unsigned int i = 0; i < samples.size(); i++) {
    //     samples[i] = alt.sample(rand);
    // }
    // std::sort (samples.begin(), samples.end());

    // for (unsigned int i = 0; i < samples.size(); i++) {
    //     printf("%u %u\n", i, samples[i]);
    // }
    
    const rt::screen test_scr(LOWRES_DEFAULT_WIDTH, LOWRES_DEFAULT_HEIGHT);
    const rt::color color_one(1.0f, 1.0f, 1.0f);
    
    while (true) {
        for (unsigned int i = 0; i < 1000000; i++) {
            unsigned int s = alt.sample(rand);
            rt::color& px = lrdata[s % LOWRES_DEFAULT_WIDTH][s / LOWRES_DEFAULT_WIDTH];
            px = px + color_one;
        }
        test_scr.fast_copy(lrdata, LOWRES_DEFAULT_WIDTH, LOWRES_DEFAULT_HEIGHT, 1);
        test_scr.update_from_texture();
    }

    // Works perfectly!
    */
    ////




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
    bool russian_roulette_enabled = false;

    if (argc == 1|| atoi(argv[1]) == 0) {
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

            if (strcmp(argv[index_arg], "-rr") == 0) {
                russian_roulette_enabled = true;
                index_arg++;
                continue;
            }
        }
    }

    if (gamma_enabled) printf("Gamma correction: %.1f\n", (1.0 / gamma));
    if (reinhardt_enabled) printf("Reinhardt local tone mapping enabled\n");
    if (russian_roulette_enabled) printf("Russian roulette technique enabled\n");

    /* Checking if the output directory exists */
    struct stat info;
    bool output_dir_exists = stat("../output", &info) == 0 && info.st_mode & S_IFDIR;

    auto create_dir = [&output_dir_exists]() {
        if (not output_dir_exists) {
            output_dir_exists = std::filesystem::create_directories("../output");
        }
    };

    /* *************************** */
    /* Scene description */

    std::optional<scene> scene_opt = parse_scene_descriptor("../scene.txt");
    
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

        const unsigned long int t_init = time_enabled ? time(0) : 0;
        unsigned long int t_export = 0;
        unsigned long int t_end = 0;
        
        for (unsigned int i = 0; i < target_number_of_rays; i++) {
            if (multisample)
                render_loop_parallel_multisample(matrix, scene, number_of_bounces, number_of_samples);
            else
                render_loop_parallel(matrix, scene, number_of_bounces, russian_roulette_enabled, i);

            //if (target_number_of_rays <= 10 || i % 10 == 9) {
                printf("\r%u / %u", i+1, target_number_of_rays);
                fflush(stdout);
            //}

            /* Exporting as rtdata every 100 samples */
            if (i % 100 == 99) {
                t_end = time_enabled ? time(0) : 0;
                create_dir();
                export_raw("../output/image.rtdata", i+1, matrix);
                const unsigned long int t_export_end = time_enabled ? time(0) : 0;
                t_export += t_export_end - t_end;
            }

        }
        if (not t_end) t_end = time(0);
        const unsigned long int elapsed = t_end - t_init - t_export;

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

        if (time_enabled) {

            if (elapsed < 60)
                printf("Total duration: %lu seconds\n", elapsed);
            else if (elapsed < 3600)
                printf("Total duration: %lu minutes %lu seconds\n", elapsed / 60, elapsed % 60);
            else
                printf("Total duration: %lu hours %lu minutes %lu seconds\n", elapsed / 3600, (elapsed % 3600) / 60, elapsed % 60);
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
            render_loop_parallel(matrix, scene, number_of_bounces, russian_roulette_enabled, 1);
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
                render_loop_parallel(matrix, scene, number_of_bounces, russian_roulette_enabled, number_of_rays);
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