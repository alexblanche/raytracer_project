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

    const int n = argc - index_source;

    printf("Merging %d files...\n", n);

    /* Checking file extensions (ugly) */
    char* p = argv[1];
    while (*p != '\0') p++;
    if (!(*(p-3) == 'b' && *(p-2) == 'm' && *(p-1) == 'p')) {
        printf("Error: first output file should have format bmp\n");
        return EXIT_FAILURE;
    }
    p = argv[2];
    while (*p != '\0') p++;
    if (!(*(p-6) == 'r' && *(p-5) == 't' && *(p-4) == 'd' && *(p-3) == 'a' && *(p-2) == 't' && *(p-1) == 'a')) {
        printf("Error: second output file should have format rtdata\n");
        return EXIT_FAILURE;
    }

    const bool success = combine_raw(argv[1], argv[2], n, argv + index_source, gamma);

    printf("Done.\n");

    if (success) {
        printf("Files %s and %s created\n", argv[1], argv[2]);
    }
    else {
        printf("Error, merger failed\n");
    }

    return EXIT_SUCCESS;
}