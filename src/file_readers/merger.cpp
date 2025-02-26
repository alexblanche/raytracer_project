#include <iostream>
#include <fstream>
#include <stdlib.h>

#include <cstring>

#include "file_readers/raw_data.hpp"

/* Program that combines raw data files into a bmp file */
/* Arguments syntax:
   ./merge dest.bmp dest.rtdata [-gamma g] source1 source2 ... sourcen
   with g the gamma value */
int main(int argc, char* argv[]) {

    if (argc <= 3) {
        printf("No source file provided\n");
        return EXIT_SUCCESS;
    }
    double gamma = 1.0;
    int index_source = 3;
    if (strcmp(argv[3], "-gamma") == 0) {
        gamma = 1.0 / atof(argv[4]);
        index_source += 2;
        printf("Gamma correction: %.1f\n", 1.0 / gamma);
    }

    printf("Merging files...\n");

    const bool success = combine_raw(argv[1], argv[2], argc - index_source, argv + index_source, gamma);

    printf("Done.\n");

    if (success) {
        printf("Files %s and %s created\n", argv[1], argv[2]);
    }
    else {
        printf("Error, merger failed\n");
    }

    return EXIT_SUCCESS;
}