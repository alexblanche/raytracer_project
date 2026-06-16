#include "file_readers/image_files/raw_data.hpp"

#include <filesystem>

/* Program that combines raw data files into a bmp file */
/* Arguments syntax:
   ./merge dest.bmp dest.rtdata [-gamma g] source1 source2 ... sourcen
   with g the gamma value */
int main(int argc, char* argv[]) {
    
    const std::vector<std::string> args(argv + 1, argv + argc);

    float gamma = 1.0f;
    unsigned int index_source = 2;
    if (args.size() > 3 && args[2] == "-gamma") {
        try {
            const float gamma_val = std::stof(args[3]);
            gamma = 1.0f / gamma_val;
            index_source += 2;
            printf("Gamma correction: %.1f\n", gamma_val);
        }
        catch (const std::exception& e) {
            printf("%s\n", e.what());
            return EXIT_FAILURE;
        }
    }

    if (args.size() <= index_source) {
        printf("Error: no source file provided\n");
        return EXIT_FAILURE;
    }

    /* Checking file extensions */
    if (!(std::filesystem::path(args[0]).extension().generic_string() == ".bmp")) {
        printf("Error: first output file should have format bmp\n");
        return EXIT_FAILURE;
    }
    if (!(std::filesystem::path(args[1]).extension().generic_string() == ".rtdata")) {
        printf("Error: second output file should have format rtdata\n");
        return EXIT_FAILURE;
    }

    const exit_status status = raw_data::combine_files(args[0], args[1], std::span(args).subspan(index_source), gamma);

    printf("Generating merged files\n");

    switch (status) {
        case exit_status::Success:
            printf("Files %s and %s created\n", args[0].c_str(), args[1].c_str());
            break;
    
        case exit_status::Failure:
            printf("Error, merger failed\n");
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}