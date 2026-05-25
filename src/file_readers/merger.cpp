#include <iostream>
#include <fstream>
#include <stdlib.h>

#include <cstring>
#include <string>
#include <filesystem>

#include "file_readers/raw_data.hpp"

/* Program that combines raw data files into a bmp file */
/* Arguments syntax:
   ./merge dest.bmp dest.rtdata [-gamma g] source1 source2 ... sourcen
   with g the gamma value */
int main(int argc, char* argv[]) {
    
    const std::vector<std::string> args(argv + 1, argv + argc);

    if (argc <= 3) {
        printf("No source file provided\n");
        return EXIT_SUCCESS;
    }

    double gamma = 1.0;
    int index_source = 3;
    if (args[2] == "-gamma") {
        gamma = 1.0f / std::stof(args[3]);
        index_source += 2;
        printf("Gamma correction: %.1f\n", 1.0f / gamma);
    }

    const int n = argc - index_source;

    printf("Merging %d files...\n", n);

    /* Checking file extensions */
    if (!(std::filesystem::path(args[0]).extension().generic_string() == ".bmp")) {
        printf("Error: first output file should have format bmp\n");
        return EXIT_FAILURE;
    }
    if (!(std::filesystem::path(args[1]).extension().generic_string() == ".rtdata")) {
        printf("Error: second output file should have format rtdata\n");
        return EXIT_FAILURE;
    }

    const exit_status status = combine_raw(args[0], args[1], n, std::span(args).last(index_source), gamma);

    printf("Done.\n");

    switch (status) {
        case exit_status::Success:
            printf("Files %s and %s created\n", args[0].c_str(), args[1].c_str());
            return EXIT_SUCCESS;
    
        case exit_status::Failure:
            printf("Error, merger failed\n");
            return EXIT_FAILURE;
    }
}