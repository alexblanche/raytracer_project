#include "file_readers/mtl_parser.hpp"

#include "scene/material/material.hpp"

#include <vector>
#include <stdio.h>
#include <string.h>
#include <string>

/* Mtl file parser */

/* Defines the new materials from the mtl file file_name,
   places their name in material_names, and places the materials in material_set
   Returns true if the operation was successful */
bool parse_mtl_file(const char* file_name,
    std::vector<std::string>& material_names, std::vector<material>& material_set) {

    FILE* file = fopen(file_name, "r");

    if (file == NULL) {
        printf("Error, .mtl file %s not found\n", file_name);
        return false;
    }

    /* Parsing loop */
    while (not feof(file)) {
        // longest items are newmtl
        char s[7];
        if (fscanf(file, "%6s ", s) != 1) {
            break;
        }

        /* Commented line, or ignored command */
        if (strcmp(s, "#") == 0) {

            char c;
            do {
                c = fgetc(file);
            }
            while (c != '\n' && c != EOF);
            ungetc(c, file);
        }
        else if (strcmp(s, "newmtl") == 0) {

        }

    }

    fclose(file);
    return true;
}
