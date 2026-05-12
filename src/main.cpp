#include <iostream>
#include <cstdlib>
#include "screen/screen.hpp"

#include "file_readers/raw_data.hpp"
#include "file_readers/bmp_reader.hpp"
#include "file_readers/scene_parser.hpp"

#include "render/render_loops.hpp"

// Folder creation
#include <sys/stat.h>
#include <filesystem>

#include <ctime>

/* ********* MAIN FUNCTION ********* */

/** Loop keys:
 * Space/Enter: Continue
 * B key:       Save as image.bmp
 * R key:       Save raw data as image.rtdata
 * Esc:         Exit
 */

struct program_parameters {
    enum class mode {
        Interactive, Offline
    };
    mode mode;
    unsigned int target_number_of_rays;
};

struct sampling_parameters {
    enum class mode {
        UniSample, MultiSample
    };
    mode mode;
    unsigned int multisample_number_of_samples;
};

struct tone_mapping_parameters {
    enum class mode {
        Disabled, Gamma, Reinhardt
    };
    mode mode;
    float gamma_value;
};

enum class time_mode {
    Disabled, Simple, Full
};

enum class russian_roulette_mode {
    Disabled, Enabled
};

struct runtime_parameters {
    unsigned int            number_of_bounces      = 2;
    program_parameters      program                = { program_parameters::mode::Interactive,   0    };
    sampling_parameters     sampling               = { sampling_parameters::mode::UniSample,    1    };
    tone_mapping_parameters tone_mapping           = { tone_mapping_parameters::mode::Disabled, 1.0f };
    time_mode               time                   = time_mode::Disabled;
    russian_roulette_mode   russian_roulette       = russian_roulette_mode::Disabled;
};

void create_dir(bool& output_dir_exists) {
    if (not output_dir_exists) {
        output_dir_exists = std::filesystem::create_directories("../output");
    }
}

bool run_offline(const runtime_parameters& runtime_parameters, std::vector<std::vector<rt::color>>& matrix, scene& scene, bool& output_dir_exists) {
    printf("Rendering...\n");
    printf("0 / %u", runtime_parameters.program.target_number_of_rays);
    fflush(stdout);

    const bool time_enabled = runtime_parameters.time != time_mode::Disabled;
    const unsigned long int t_init = time_enabled ? time(0) : 0;
    unsigned long int t_export = 0;
    unsigned long int t_end = 0;

    const unsigned int target = runtime_parameters.program.target_number_of_rays;
    const bool russian_roulette = runtime_parameters.russian_roulette == russian_roulette_mode::Enabled;

    for (unsigned int i = 0; i < target; i++) {
        switch (runtime_parameters.sampling.mode) {
            case sampling_parameters::mode::MultiSample:
                render_loop_parallel_multisample(
                    matrix,
                    scene,
                    runtime_parameters.number_of_bounces,
                    runtime_parameters.sampling.multisample_number_of_samples
                );
                break;
            case sampling_parameters::mode::UniSample:
                render_loop_parallel(
                    matrix,
                    scene,
                    runtime_parameters.number_of_bounces,
                    russian_roulette,
                    i
                );
                break;
        }

        printf("\r%u / %u", i + 1, target);
        fflush(stdout);

        /* Exporting as rtdata every EXPORT_INTERVAL samples */
        constexpr unsigned int EXPORT_INTERVAL = 1000;
        if ((i + 1) % EXPORT_INTERVAL == 0) {
            t_end = time_enabled ? time(0) : 0;
            create_dir(output_dir_exists);
            export_raw("../output/image.rtdata", i+1, matrix);
            const unsigned long int t_export_end = time_enabled ? time(0) : 0;
            t_export += t_export_end - t_end;
        }

    }
    if (not t_end) t_end = time(0);
    const unsigned long int elapsed = t_end - t_init - t_export;

    printf("\r%u / %u", target, target);

    create_dir(output_dir_exists);
    const bool success_bmp = write_bmp("../output/image.bmp", matrix, target, runtime_parameters.tone_mapping.gamma_value);
    if (not success_bmp) {
        printf("Save failed\n");
        return false;
    }
    printf(" Saved as output/image.bmp\n");

    if (time_enabled) {

        if (elapsed < 60)
            printf("Total duration: %lu seconds\n", elapsed);
        else if (elapsed < 3600)
            printf("Total duration: %lu minutes %lu seconds\n", elapsed / 60, elapsed % 60);
        else
            printf("Total duration: %lu hours %lu minutes %lu seconds\n", elapsed / 3600, (elapsed % 3600) / 60, elapsed % 60);
    }
        
    //export_raw("image.rtdata", target_number_of_rays, matrix);
    return true;
}


// #include "file_readers/hdr_reader.hpp"
// #include "scene/light_sources/infinite_area.hpp"

int main(int argc, char *argv[]) {

    // printf("real : %llu, color : %llu, vector : %llu\n", sizeof(real), sizeof(rt::color), sizeof(rt::vector));
    // printf("real : %llu\n", sizeof(real));
    // printf("size_t : %llu ; unsigned int : %llu\n", sizeof(size_t), sizeof(unsigned int));
    // printf("unsigned short : %llu ; unsigned char : %llu\n", sizeof(unsigned short int), sizeof(unsigned char));
    // printf("material : %llu with alignment %llu\n", sizeof(material), alignof(material));
    // printf("object : %llu with alignment %llu\n", sizeof(object), alignof(object));
    // printf("texture_info : %llu with alignment %llu\n", sizeof(texture_info), alignof(texture_info));
    // printf("triangle : %llu with alignment %llu\n", sizeof(triangle), alignof(triangle));
    // printf("sphere : %llu with alignment %llu\n", sizeof(sphere), alignof(sphere));
    // printf("vector : %llu with alignment %llu\n", sizeof(rt::vector), alignof(rt::vector));
    // printf("plane : %llu with alignment %llu\n", sizeof(plane), alignof(plane));
    // printf("\n");

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
    

    constexpr char const* const default_filename = "../scene.txt";
    const char* filename = default_filename;

    runtime_parameters runtime_parameters;

    if (argc > 1 && !atoi(argv[1])) {
        // file specified

        filename = argv[1];
        if (std::filesystem::is_regular_file(filename)) {
            argc --;
            argv ++;
        }
        else {
            printf("Scene description %s not found\n", filename);
            return EXIT_FAILURE;
        }
    }

    printf("Scene descriptor: %s\n", std::filesystem::path(filename).filename().generic_string().data());

    if (argc == 1 || atoi(argv[1]) == 0) {
        printf("Number of bounces: %u (default)\n", runtime_parameters.number_of_bounces);
    }
    else {
        runtime_parameters.number_of_bounces = atoi(argv[1]);
        printf("Number of bounces: %u\n", runtime_parameters.number_of_bounces);
    }

    if (argc > 2) {

        int index_arg = 2;
        while (index_arg < argc) {

            const std::string arg = argv[index_arg];

            if (arg.compare("-time") == 0) {
                runtime_parameters.time = time_mode::Simple;
                index_arg++;

                if (index_arg < argc) {
                    if (std::string(argv[index_arg]).compare("all") == 0) {
                        runtime_parameters.time = time_mode::Full;
                        index_arg++;
                    }
                }
                continue;
            }

            if (arg.compare("-rays") == 0) {
                runtime_parameters.program.mode = program_parameters::mode::Offline;
                index_arg++;

                if (index_arg < argc) {
                    runtime_parameters.program.target_number_of_rays = atoi(argv[index_arg]);
                    index_arg++;
                }
                else {
                    printf("Error, -rays option expects 1 argument\n");
                    return EXIT_FAILURE;
                }
                continue;
            }

            if (arg.compare("-multisample") == 0) {
                runtime_parameters.sampling.mode = sampling_parameters::mode::MultiSample;
                index_arg++;

                if (index_arg < argc) {
                    runtime_parameters.sampling.multisample_number_of_samples = atoi(argv[index_arg]);
                    index_arg++;
                }
                else {
                    printf("Error, -multisample option expects 1 argument\n");
                    return EXIT_FAILURE;
                }
                continue;
            }

            if (arg.compare("-gamma") == 0) {
                if (runtime_parameters.tone_mapping.mode != tone_mapping_parameters::mode::Reinhardt) {
                    runtime_parameters.tone_mapping.mode = tone_mapping_parameters::mode::Gamma;
                }
                index_arg++;

                if (index_arg < argc) {
                    runtime_parameters.tone_mapping.gamma_value = 1.0f / atof(argv[index_arg]);
                    index_arg++;
                }
                else {
                    printf("Error, -gamma option expects 1 argument\n");
                    return EXIT_FAILURE;
                }

                continue;
            }

            if (arg.compare("-reinhardt") == 0) {
                runtime_parameters.tone_mapping.mode = tone_mapping_parameters::mode::Reinhardt;
                index_arg++;
                continue;
            }

            if (arg.compare("-rr") == 0) {
                runtime_parameters.russian_roulette = russian_roulette_mode::Enabled;
                index_arg++;
                continue;
            }

            printf("Error, incorrect argument %s\n", arg.data());
            return EXIT_FAILURE;
        }
    }

    bool gamma_enabled = runtime_parameters.tone_mapping.mode != tone_mapping_parameters::mode::Disabled;
    float gamma = runtime_parameters.tone_mapping.gamma_value;

    if (gamma_enabled)
        printf("Gamma correction: %.1f\n", (1.0f / gamma));

    if (runtime_parameters.tone_mapping.mode == tone_mapping_parameters::mode::Reinhardt)
        printf("Reinhardt local tone mapping enabled\n");

    if (runtime_parameters.russian_roulette == russian_roulette_mode::Enabled)
        printf("Russian roulette technique enabled\n");

    /* Checking if the output directory exists */
    struct stat info;
    bool output_dir_exists = stat("../output", &info) == 0 && info.st_mode & S_IFDIR;

    /* *************************** */
    /* Scene description */

    std::optional<scene> scene_opt = parse_scene_descriptor(filename);
    
    if (not scene_opt.has_value()) {
        printf("Scene creation failed\n");
        return EXIT_FAILURE;
    }

    scene& scene = scene_opt.value();
    if (scene.gamma != 1.0f) {
        if (gamma_enabled && scene.gamma != gamma) printf("Warning: gamma correction value passed by command-line argument (%.1f) was overwritten by scene description (%.1f)\n",
            gamma, (1.0f / scene.gamma));

        if (runtime_parameters.tone_mapping.mode == tone_mapping_parameters::mode::Disabled) {
            runtime_parameters.tone_mapping.mode = tone_mapping_parameters::mode::Gamma;
        }
        runtime_parameters.tone_mapping.gamma_value = scene.gamma;
        gamma_enabled = true;
        gamma = scene.gamma;
        printf("Gamma correction: %.1f\n", (1.0f / gamma));
    }

    printf("Number of objects: %zu\n", scene.object_set.size());


    /**************************************************************************/

    /* Definition of the matrix in which we will write the image */
    std::vector<std::vector<rt::color>> matrix(scene.width, std::vector<rt::color>(scene.height));

    /* Generating an image of target_number_of_rays rays */

    if (runtime_parameters.program.mode == program_parameters::mode::Offline) {

        const bool ok = run_offline(runtime_parameters, matrix, scene, output_dir_exists);
        return ok ? EXIT_SUCCESS : EXIT_FAILURE;
    }


    /* Interactive mode */

    printf("Initialization complete, computing the first ray...");
    fflush(stdout);

    const bool russian_roulette = runtime_parameters.russian_roulette == russian_roulette_mode::Enabled;

    auto render = [&] (unsigned int iter) {
        switch (runtime_parameters.time) {
            case time_mode::Simple:
                render_loop_parallel_time(matrix, scene, runtime_parameters.number_of_bounces, false);
                break;
            case time_mode::Full:
                render_loop_parallel_time(matrix, scene, runtime_parameters.number_of_bounces, true);
                break;
            case time_mode::Disabled:
                switch (runtime_parameters.sampling.mode) {
                    case sampling_parameters::mode::MultiSample:
                        render_loop_parallel_multisample(
                            matrix,
                            scene,
                            runtime_parameters.number_of_bounces,
                            runtime_parameters.sampling.multisample_number_of_samples
                        );
                        break;
                    case sampling_parameters::mode::UniSample:
                        render_loop_parallel(
                            matrix,
                            scene,
                            runtime_parameters.number_of_bounces,
                            russian_roulette,
                            iter
                        );
                        break;
                }
                break;
        }
    };
    render(1);

    const rt::screen scr(scene.width, scene.height);
    
    auto copy_to_texture = [&] (unsigned int iter) {
        switch (runtime_parameters.tone_mapping.mode) {
            case tone_mapping_parameters::mode::Disabled:
                scr.fast_copy(matrix, scene.width, scene.height, iter);
                break;
            case tone_mapping_parameters::mode::Gamma:
                scr.fast_copy_gamma(matrix, scene.width, scene.height, iter, gamma);
                break;
            case tone_mapping_parameters::mode::Reinhardt:
                scr.fast_copy_reinhardt(matrix, scene.width, scene.height, iter, gamma);
                break;
        }
    };

    copy_to_texture(1);
    scr.update_from_texture();

    printf("\r                                                   ");
    printf("\rNumber of samples per pixel: 1");
    fflush(stdout);

    for (unsigned int number_of_rays = 2; number_of_rays <= MAX_RAYS; number_of_rays++) {

        render(number_of_rays);

        printf("\rNumber of samples per pixel: %u", number_of_rays);
        fflush(stdout);

        copy_to_texture(number_of_rays);
        scr.update_from_texture();

        const rt::screen::key key = scr.poll_keyboard_event();
        switch (key) {
            case rt::screen::key::QuitEvent:
                /* Esc or the window exit "X" clicked */
                printf("\n");
                return EXIT_SUCCESS;
            case rt::screen::key::B:
                /* B */
                /* Export as BMP */
                {
                    create_dir(output_dir_exists);
                    const bool success_bmp = write_bmp("../output/image.bmp", matrix, number_of_rays, gamma);
                    if (not success_bmp) {
                        printf("Save failed\n");
                        return EXIT_FAILURE;
                    }
                    printf(" Saved as output/image.bmp\n");
                    break;
                }
            case rt::screen::key::R:
                /* R */
                /* Export raw data */
                {
                    create_dir(output_dir_exists);
                    const bool success_raw = export_raw("../output/image.rtdata", number_of_rays, matrix);
                    if (not success_raw) {
                        printf("Save failed\n");
                        return EXIT_FAILURE;
                    }
                    printf(" Saved as output/image.rtdata\n");
                    break;
                }
            default:
                break;
        }
    }

    create_dir(output_dir_exists);
    const bool success =
            write_bmp ("../output/image_final.bmp", matrix, MAX_RAYS, gamma)
         && export_raw("../output/image_final.rtdata", MAX_RAYS, matrix);
    
    if (not success) {
        printf("\nSave failed\n");
        return EXIT_FAILURE;
    }

    printf("\nSaved as output/image_final.bmp and output/image_final.rtdata\n");
    return EXIT_SUCCESS;
}