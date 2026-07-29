#include "file_readers/image_files/raw_data.hpp"

#include <filesystem>

using namespace std::filesystem;

static void check_directory(const std::string& dir_name, bool& dir_is_set) {
    if (exists(dir_name) && is_directory(dir_name))
        dir_is_set = true;
    else
        throw std::runtime_error(("Error: cannot find directory " + dir_name + "\n").c_str());
}

struct parameters {
    std::string input_dir;
    std::string output_dir;
    std::string dest_bmp = "merged.bmp";
    std::string dest_raw = "merged.rtdata";
    std::optional<float> gamma_opt;
    bool input_dir_set  = false;
    bool output_dir_set = false;
    bool dest_set       = false;
    bool gamma_set      = false;
};

/* Returns the index in the argument list after the parsing, at the beginning of the source filenames */
static unsigned int parse_parameters(parameters& param, const std::span<const std::string> args) {

    const unsigned int args_size = args.size();
    unsigned int current_index = 0;

    while (args_size > current_index && args[current_index].starts_with('-')) {
        const std::string& arg = args[current_index];
        if (int nb_args = 1;
            (arg == "-input" || arg == "-I" || arg == "-IO")
                && args_size > current_index + nb_args
                && not param.input_dir_set) {

            param.input_dir = args[current_index + 1];
            check_directory(param.input_dir, param.input_dir_set);

            if (arg == "-IO") {
                param.output_dir = param.input_dir;
                param.output_dir_set = true;
            }

            current_index += nb_args + 1;
        }
        else if (int nb_args = 1;
            (arg == "-output" || arg == "-O")
                && args_size > current_index + nb_args
                && not param.output_dir_set) {
            
            param.output_dir = args[current_index + 1];
            check_directory(param.output_dir, param.output_dir_set);

            current_index += nb_args + 1;
        }
        else if (int nb_args = 2;
            arg == "-dest"
                && args_size > current_index + nb_args
                && not param.dest_set) {

            const std::string& dest1 = args[current_index + 1];
            const std::string& dest2 = args[current_index + 2];
            if (dest1.ends_with(".bmp") && dest2.ends_with(".rtdata")) {
                param.dest_bmp = dest1;
                param.dest_raw = dest2;
            }
            else if (dest2.ends_with(".bmp") && dest1.ends_with(".rtdata")) {
                param.dest_bmp = dest2;
                param.dest_raw = dest1;
            }
            else
                throw std::runtime_error("Error: destination files should have formats .bmp and .rtdata\n");
            
            param.dest_set = true;
            current_index += nb_args + 1;
        }
        else if (int nb_args = 1;
            arg == "-gamma"
                && args_size > current_index + nb_args
                && not param.gamma_set) {

            const float gamma_val = std::stof(args[current_index + 1]);
            param.gamma_opt = 1.0f / gamma_val;
            printf("Gamma correction: %.1f\n", gamma_val);

            param.gamma_set = true;
            current_index += nb_args + 1;
        }
    }

    if (args_size <= current_index)
        throw std::runtime_error("Error: no source file provided\n");

    return current_index;
}


/* Program that combines raw data files into a bmp file */
/* Arguments syntax:
   ./merge dest.bmp dest.rtdata [-gamma g] source1 source2 ... sourcen
   with g the gamma value */
int main(int argc, char* argv[]) {
    
    const std::vector<std::string> args(argv + 1, argv + argc);
    parameters param;

    unsigned int current_index = 0;
    try {
        current_index = parse_parameters(param, args);
    }
    catch (const std::exception& e) {
        printf("%s\n", e.what());
        return EXIT_FAILURE;
    }

    const std::string dest_bmp = param.output_dir_set ?
          path(param.output_dir).append(param.dest_bmp).generic_string()
        : param.dest_bmp;
    const std::string dest_raw = param.output_dir_set ?
          path(param.output_dir).append(param.dest_raw).generic_string()
        : param.dest_raw;

    const std::optional<std::string> input_dir = param.input_dir_set ?
          std::optional(param.input_dir)
        : std::nullopt;

    const exit_status status = raw_data::combine_files(dest_bmp, dest_raw, std::span(args).subspan(current_index), input_dir, param.gamma_opt);

    switch (status) {
        case exit_status::Success:
            printf("Files %s and %s created\n", param.dest_bmp.c_str(), param.dest_raw.c_str());
            break;
    
        case exit_status::Failure:
            printf("Error: merger failed\n");
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}