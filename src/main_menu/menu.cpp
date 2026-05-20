#include "main_menu/menu.hpp"
#include "screen/screen.hpp"
#include "render/render_loops.hpp"

#include <ctime>
#include <filesystem>

static bool is_number(const std::string& s) {
    try {
        (void) std::stoul(s);
        return true;
    }
    catch (std::exception& e) {
        return false;
    }
}

static bool is_float(const std::string& s) {
    try {
        (void) std::stof(s);
        return true;
    }
    catch (std::exception& e) {
        return false;
    }
}

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

exit_status menu::parse_aux(const std::vector<std::string>& args, const unsigned int index_arg) {

    const unsigned int size = args.size();

    for (unsigned int i = index_arg; i < size; i++) {

        const std::string& arg = args[i];
        switch(match(arg)) {

            case cli_argument::Time: {
                runtime_parameters.time = time_mode::Simple;
                if (i + 1 < size && match(args[i + 1]) == cli_argument::TimeAll)
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
                if (runtime_parameters.tone_mapping.mode != tone_mapping_parameters::mode::Reinhardt)
                    runtime_parameters.tone_mapping.mode = tone_mapping_parameters::mode::Gamma;

                if (i + 1 >= size || not is_float(args[i + 1])) {
                    printf("Error, -gamma option expects 1 argument\n");
                    return exit_status::Failure;
                }
                const std::string& next = args[++i];
                runtime_parameters.tone_mapping.gamma_value = 1.0f / std::stof(next);
                break;
            }

            case cli_argument::Reinhardt: {
                runtime_parameters.tone_mapping.mode = tone_mapping_parameters::mode::Reinhardt;
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

exit_status menu::parse_arguments(int argc, char **argv) {

    const std::vector<std::string> args(argv + 1, argv + argc);
    const unsigned int size = args.size();
    unsigned int index = 0;

    // Scene descriptor
    const bool descriptor_specified = index < size && not is_number(args[index]);
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
    if (index < size && is_number(args[index])) {
        runtime_parameters.number_of_bounces = std::stoul(args[index]);
        printf("Number of bounces: %u\n", runtime_parameters.number_of_bounces);
        index++;
    }
    else {
        printf("Number of bounces: %u (default)\n", runtime_parameters.number_of_bounces);
    }

    // Other arguments
    if (index < size) {
        const exit_status status = parse_aux(args, index);
        if (status == exit_status::Failure)
            return exit_status::Failure;
    }

    // Display
    switch (runtime_parameters.tone_mapping.mode) {
        case tone_mapping_parameters::mode::Reinhardt:
            printf("Reinhardt local tone mapping enabled\n");
        case tone_mapping_parameters::mode::Gamma:
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
    const bool gamma_disabled = runtime_parameters.tone_mapping.mode == tone_mapping_parameters::mode::Disabled;

    if (new_gamma != 1.0f) {
        if ((not gamma_disabled) && new_gamma != gamma)
            printf("Warning: gamma correction value passed by command-line argument (%.1f) was overwritten by scene description (%.1f)\n",
                gamma, (1.0f / new_gamma));

        if (gamma_disabled)
            runtime_parameters.tone_mapping.mode = tone_mapping_parameters::mode::Gamma;

        gamma = new_gamma;
        printf("Gamma correction: %.1f\n", (1.0f / gamma));
    }
}

static inline void render_simple(std::vector<std::vector<rt::color>>& matrix, const scene& scene,
    const runtime_parameters& runtime_parameters, const unsigned int iter) {

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
                runtime_parameters.russian_roulette,
                iter
            );
            break;
    }
}

static inline void render(std::vector<std::vector<rt::color>>& matrix, const scene& scene,
    const runtime_parameters& runtime_parameters, const unsigned int iter) {
    
    switch (runtime_parameters.time) {
        case time_mode::Simple:
        case time_mode::Full:
            render_loop_parallel_time(matrix, scene, runtime_parameters.number_of_bounces, runtime_parameters.time);
            break;
        
        case time_mode::Disabled:
            render_simple(matrix, scene, runtime_parameters, iter);
            break;
    }
}

class timer {
    private:
        bool time_enabled;
        unsigned long int t_init;
        unsigned long int t_export = 0;
        unsigned long int t_end    = 0;
        unsigned long int elapsed  = 0;

    public:
        timer(time_mode mode)
            : time_enabled(mode != time_mode::Disabled) {}

        inline void start() {
            t_init = time_enabled ? time(0) : 0;
        }

        inline void interrupt() {
            t_end  = time_enabled ? time(0) : 0;
        }

        inline void resume() {
            const unsigned long int t_export_end = time_enabled ? time(0) : 0;
            t_export += t_export_end - t_end;
        }

        inline void stop() {
            t_end = ((not time_enabled) || t_end) ? t_end : time(0);
            elapsed = t_end - t_init - t_export;
        }

        void print() const {
            if (not time_enabled)
                return;
            if (elapsed < 60)
                printf("Total duration: %lu seconds\n", elapsed);
            else if (elapsed < 3600)
                printf("Total duration: %lu minutes %lu seconds\n", elapsed / 60, elapsed % 60);
            else
                printf("Total duration: %lu hours %lu minutes %lu seconds\n", elapsed / 3600, (elapsed % 3600) / 60, elapsed % 60);
        }
};

static exit_status run_offline(const runtime_parameters& runtime_parameters, std::vector<std::vector<rt::color>>& matrix,
    const scene& scene, const file_handler& file_handler) {

    const unsigned int target = runtime_parameters.program.target_number_of_rays;

    printf("Rendering...\n");
    printf("0 / %u", target);
    fflush(stdout);

    timer timer(runtime_parameters.time);
    timer.start();

    for (unsigned int i = 0; i < target; i++) {

        render_simple(matrix, scene, runtime_parameters, i);

        printf("\r%u / %u", i + 1, target);
        fflush(stdout);

        /* Exporting as rtdata every EXPORT_INTERVAL samples */
        constexpr unsigned int EXPORT_INTERVAL = 1000;
        if ((i + 1) % EXPORT_INTERVAL == 0) {
            timer.interrupt();

            const exit_status status = file_handler.export_raw_data("image.rtdata", i + 1, matrix, runtime_parameters);
            if (status == exit_status::Failure)
                return exit_status::Failure;
            
            timer.resume();
        }
    }

    timer.stop();
    printf("\r%u / %u\n", target, target);
    timer.print();

    return file_handler.export_bmp("image.bmp", target, matrix, runtime_parameters);
}

// Returns an exit_status if the program has to stop, either because of a failure or because a quit event happened
static std::optional<exit_status> process_events(const rt::screen& scr, const file_handler& file_handler,
        const std::vector<std::vector<rt::color>>& matrix, const unsigned int number_of_rays,
        const runtime_parameters& runtime_parameters) {

    switch (scr.poll_keyboard_event()) {

        case rt::screen::key::QuitEvent:
            /* Esc or the window exit "X" clicked */
            printf("\n");
            return exit_status::Success;

        case rt::screen::key::B: {
            /* B */
            printf(" ");
            const exit_status status = file_handler.export_bmp("image.bmp", number_of_rays, matrix, runtime_parameters);
            if (status == exit_status::Failure)
                return exit_status::Failure;
            break;
        }
        case rt::screen::key::R: {
            /* R */
            printf(" ");
            const exit_status status = file_handler.export_raw_data("image.rtdata", number_of_rays, matrix, runtime_parameters);
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
    std::vector<std::vector<rt::color>>& matrix, const scene& scene, const file_handler& file_handler) {

    printf("Initialization complete, computing the first ray...");
    fflush(stdout);

    const float gamma = runtime_parameters.tone_mapping.gamma_value;

    render(matrix, scene, runtime_parameters, 1);

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

        render(matrix, scene, runtime_parameters, number_of_rays);

        printf("\rNumber of samples per pixel: %u", number_of_rays);
        fflush(stdout);

        copy_to_texture(number_of_rays);
        scr.update_from_texture();

        const std::optional<exit_status> status = process_events(scr, file_handler, matrix, number_of_rays, runtime_parameters);
        if (status.has_value())
            return status.value();
    }

    return file_handler.export_bmp(     "image_final.bmp",    MAX_RAYS, matrix, runtime_parameters)
        && file_handler.export_raw_data("image_final.rtdata", MAX_RAYS, matrix, runtime_parameters);
}

exit_status menu::run(const scene& scene) const {

    std::vector<std::vector<rt::color>> matrix(scene.width, std::vector<rt::color>(scene.height));
    const file_handler file_handler;

    switch (runtime_parameters.program.mode) {
        case program_parameters::mode::Offline:
            return run_offline(runtime_parameters, matrix, scene, file_handler);
        
        case program_parameters::mode::Interactive:
            return run_interactive(runtime_parameters, matrix, scene, file_handler);
    }
}