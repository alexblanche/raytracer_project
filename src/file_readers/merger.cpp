#include <iostream>
#include <fstream>
#include <stdlib.h>

#include "file_readers/raw_data.hpp"

/* Program that combines raw data files into a bmp file */
/* Arguments syntax:
   ./merger dest.bmp source1 source2 ... sourcen */
int main(int argc, char* argv[]) {

    if (argc <= 2) {
        printf("No source file provided\n");
        return EXIT_SUCCESS;
    }

    printf("Merging files...\n");

    const bool success = combine_raw(argv[1], argc - 2, argv + 2);

    printf("Done.\n");

    if (success) {
        printf("File %s created\n", argv[1]);
    }
    else {
        printf("Error, merger failed\n");
    }

    return EXIT_SUCCESS;
}