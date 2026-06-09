#include "main_menu/menu.hpp"
#include "main_menu/file_handler.hpp"
#include "screen/screen.hpp"
#include "render/render_loops.hpp"
#include "auxiliary/timer.hpp"

#include <string>
#include <span>
#include <filesystem>

constexpr unsigned int EXPORT_INTERVAL = 1000;
constexpr std::string DEFAULT_OUTPUT_FILE_NAME       = "image";
constexpr std::string DEFAULT_OUTPUT_FINAL_FILE_NAME = "image_final";


static constexpr std::string bmp(const std::string& file_name) {
    return file_name + ".bmp";
}
static constexpr std::string raw(const std::string& file_name) {
    return file_name + ".rtdata";
}

static bool is_number(const std::string& s) {
    try {
        std::ignore = std::stoul(s);
        return true;
    }
    catch (std::exception& e) {
        return false;
    }
}

static bool is_float(const std::string& s) {
    try {
        std::ignore = std::stof(s);
        return true;
    }
    catch (std::exception& e) {
        return false;
    }
}

/////////////////////////////////////////////////////////////////////////////////


enum class cli_argument {
    Time, TimeAll, Rays, Multisample, Gamma, Reinhardt, RussianRoulette, None
};

struct arg_pair {
    std::string  keyword;
    cli_argument arg;
};

static cli_argument match(const std::string& input) {
    static const std::vector<arg_pair> keywords = {
        { "-time",        cli_argument::Time            },
        { "all",          cli_argument::TimeAll         },
        { "-rays",        cli_argument::Rays            },
        { "-multisample", cli_argument::Multisample     },
        { "-gamma",       cli_argument::Gamma           },
        { "-reinhardt",   cli_argument::Reinhardt       },
        { "-rr",          cli_argument::RussianRoulette }
    };
    for (const auto& [ keyword, value ] : keywords) {
        if (input == keyword)
            return value;
    }
    return cli_argument::None;
}

exit_status menu::parse_aux(const std::span<const std::string> args) {

    const unsigned int size = args.size();

    for (unsigned int i = 0; i < size; i++) {

        const std::string& arg = args[i];
        switch (match(arg)) {

            case cli_argument::Time: {
                runtime_parameters.time = time_mode::Simple;
                if (i + 1 >= size)
                    break;
                const std::string& next = args[++i];
                if (match(next) == cli_argument::TimeAll)
                    runtime_parameters.time = time_mode::Full;
                break;
            }

            case cli_argument::Rays: {
                runtime_parameters.program.mode = program_parameters::mode::Offline;

                if (i + 1 >= size || not is_number(args[i + 1])) {
                    printf("Error, -rays option expects 1 argument\n");
                    return exit_status::Failure;
                }
                const std::string& next = args[++i];
                runtime_parameters.program.target_number_of_rays = std::stoul(next);
                break;
            }

            case cli_argument::Multisample: {
                runtime_parameters.sampling.mode = sampling_parameters::mode::MultiSample;

                if (i + 1 >= size || not is_number(args[i + 1])) {
                    printf("Error, -multisample option expects 1 argument\n");
                    return exit_status::Failure;
                }
                const std::string& next = args[++i];
                runtime_parameters.sampling.multisample_number_of_samples = std::stoul(next);
                break;
            }

            case cli_argument::Gamma: {
                if (runtime_parameters.tone_mapping.tm_mode != tone_mapping_parameters::mode::Reinhardt)
                    runtime_parameters.tone_mapping.tm_mode = tone_mapping_parameters::mode::Gamma;

                if (i + 1 >= size || not is_float(args[i + 1])) {
                    printf("Error, -gamma option expects 1 argument\n");
                    return exit_status::Failure;
                }
                const std::string& next = args[++i];
                runtime_parameters.tone_mapping.gamma_value = 1.0f / std::stof(next);
                break;
            }

            case cli_argument::Reinhardt: {
                runtime_parameters.tone_mapping.tm_mode = tone_mapping_parameters::mode::Reinhardt;
                break;
            }

            case cli_argument::RussianRoulette: {
                runtime_parameters.russian_roulette = russian_roulette_mode::Enabled;
                break;
            }

            default: {
                printf("Error, incorrect argument %s\n", arg.c_str());
                return exit_status::Failure;
            }
        }
    }

    return exit_status::Success;
}

exit_status menu::parse_arguments(const std::span<const std::string> args) {
    
    unsigned int index = 0;

    // Scene descriptor
    const bool descriptor_specified = index < args.size() && not is_number(args[index]);
    if (descriptor_specified) {
        scene_descriptor_name = args[index];
        if (not std::filesystem::is_regular_file(scene_descriptor_name)) {
            printf("Scene description %s not found\n", scene_descriptor_name.c_str());
            return exit_status::Failure;
        }
        index++;
    }
    printf("Scene descriptor: %s\n", std::filesystem::path(scene_descriptor_name).filename().generic_string().c_str());

    // Number of bounces
    if (index < args.size() && is_number(args[index])) {
        runtime_parameters.number_of_bounces = std::stoul(args[index]);
        printf("Number of bounces: %u\n", runtime_parameters.number_of_bounces);
        index++;
    }
    else {
        printf("Number of bounces: %u (default)\n", runtime_parameters.number_of_bounces);
    }

    // Other arguments
    if (index < args.size()) {
        const exit_status status = parse_aux(std::span(args).subspan(index));
        if (status == exit_status::Failure)
            return exit_status::Failure;
    }

    // Display
    using enum tone_mapping_parameters::mode;
    switch (runtime_parameters.tone_mapping.tm_mode) {
        case Reinhardt:
            printf("Reinhardt local tone mapping enabled\n");
        case Gamma:
            printf("Gamma correction: %.1f\n", (1.0f / runtime_parameters.tone_mapping.gamma_value));
            break;
        default:
            break;
    }

    if (runtime_parameters.russian_roulette == russian_roulette_mode::Enabled)
        printf("Russian roulette technique enabled\n");

    return exit_status::Success;
}

void menu::update_gamma(const float new_gamma) {

    float& gamma = runtime_parameters.tone_mapping.gamma_value;
    using enum tone_mapping_parameters::mode;
    const bool gamma_disabled = runtime_parameters.tone_mapping.tm_mode == Disabled;

    if (new_gamma != 1.0f) {
        if ((not gamma_disabled) && new_gamma != gamma)
            printf("Warning: gamma correction value passed by command-line argument (%.1f) was overwritten by scene description (%.1f)\n",
                gamma, (1.0f / new_gamma));

        if (gamma_disabled)
            runtime_parameters.tone_mapping.tm_mode = Gamma;

        gamma = new_gamma;
        printf("Gamma correction: %.1f\n", (1.0f / gamma));
    }
}

static inline void render_simple(image& image, const scene& scene,
    const runtime_parameters& runtime_parameters) {

    using enum sampling_parameters::mode;
    switch (runtime_parameters.sampling.mode) {
        case MultiSample:
            render_loop_parallel_multisample(
                image,
                scene,
                runtime_parameters.number_of_bounces,
                runtime_parameters.sampling.multisample_number_of_samples
            );
            break;
        case UniSample:
            render_loop_parallel(
                image,
                scene,
                runtime_parameters.number_of_bounces,
                runtime_parameters.russian_roulette
            );
            break;
    }
}

static inline void render(image& image, const scene& scene,
    const runtime_parameters& runtime_parameters) {
    
    using enum time_mode;
    switch (runtime_parameters.time) {
        case Simple:
        case Full:
            render_loop_parallel_time(image, scene, runtime_parameters.number_of_bounces, runtime_parameters.time);
            break;
        
        case Disabled:
            render_simple(image, scene, runtime_parameters);
            break;
    }
}

static exit_status run_offline(const runtime_parameters& runtime_parameters, image& image,
    const scene& scene, const file_handler& file_handler) {

    const unsigned int target = runtime_parameters.program.target_number_of_rays;

    printf("Rendering...\n");
    printf("0 / %u", target);
    fflush(stdout);

    timer timer(runtime_parameters.time);
    timer.start();

    ///////
    constexpr bool ALL_AT_ONCE_EXPERIMENT = false;
    if constexpr (ALL_AT_ONCE_EXPERIMENT) {
        render_loop_parallel_all_at_once(image, scene, runtime_parameters.number_of_bounces,
            runtime_parameters.russian_roulette, runtime_parameters.program.target_number_of_rays);
        timer.stop();
        printf("\n");
        timer.print();
        return file_handler.export_bmp(bmp(DEFAULT_OUTPUT_FILE_NAME), image);
    }
    ///////
    
    for (unsigned int i = 0; i < target; i++) {

        render_simple(image, scene, runtime_parameters);

        printf("\r%u / %u", i + 1, target);
        fflush(stdout);

        /* Exporting as rtdata every EXPORT_INTERVAL samples */
        if ((i + 1) % EXPORT_INTERVAL == 0) {
            timer.interrupt();

            printf(" ");
            const exit_status status = file_handler.export_raw_data(raw(DEFAULT_OUTPUT_FILE_NAME), image);
            if (status == exit_status::Failure)
                return exit_status::Failure;
            
            timer.resume();
        }
    }
    

    timer.stop();
    printf("\n");
    timer.print();

    return file_handler.export_bmp(bmp(DEFAULT_OUTPUT_FILE_NAME), image);
}

// Returns an exit_status if the program has to stop, either because of a failure or because a quit event happened
static std::optional<exit_status> process_events(const rt::screen& scr, const file_handler& file_handler, const image& image) {

    using enum rt::screen::key;
    switch (scr.poll_keyboard_event()) {

        case QuitEvent:
            /* Esc or the window exit "X" clicked */
            printf("\n");
            return exit_status::Success;

        case B: {
            printf(" ");
            const exit_status status = file_handler.export_bmp(bmp(DEFAULT_OUTPUT_FILE_NAME), image);
            if (status == exit_status::Failure)
                return exit_status::Failure;
            break;
        }
        case R: {
            printf(" ");
            const exit_status status = file_handler.export_raw_data(raw(DEFAULT_OUTPUT_FILE_NAME), image);
            if (status == exit_status::Failure)
                return exit_status::Failure;
            break;
        }

        default:
            break;
    }
    return std::nullopt;
}


static exit_status run_interactive(const runtime_parameters& runtime_parameters,
    image& image, const scene& scene, const file_handler& file_handler) {

    printf("Initialization complete, computing the first ray...");
    fflush(stdout);

    render(image, scene, runtime_parameters);

    const rt::screen scr(image, runtime_parameters.tone_mapping.tm_mode);

    scr.copy_to_texture(1);
    scr.update_from_texture();

    printf("\r                                                   ");
    printf("\rNumber of samples per pixel: 1");
    fflush(stdout);

    for (unsigned int number_of_rays = 2; number_of_rays <= MAX_RAYS; number_of_rays++) {

        render(image, scene, runtime_parameters);

        printf("\rNumber of samples per pixel: %u", number_of_rays);
        fflush(stdout);

        scr.copy_to_texture(number_of_rays);
        scr.update_from_texture();

        const std::optional<exit_status> status = process_events(scr, file_handler, image);
        if (status.has_value())
            return status.value();
    }

    return file_handler.export_bmp(     bmp(DEFAULT_OUTPUT_FINAL_FILE_NAME), image)
        && file_handler.export_raw_data(raw(DEFAULT_OUTPUT_FINAL_FILE_NAME), image);
}

exit_status menu::run(const scene& scene) const {

    image image(scene.width, scene.height, scene.gamma);
    const file_handler file_handler;

    using enum program_parameters::mode;
    switch (runtime_parameters.program.mode) {
        case Offline:
            return run_offline(runtime_parameters, image, scene, file_handler);
        
        case Interactive:
            return run_interactive(runtime_parameters, image, scene, file_handler);
    }
}