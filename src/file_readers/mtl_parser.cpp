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
            /* Generating a new material */
            std::string m_name(65, '\0');
            int ret = fscanf(file, " %64s", (char*) m_name.data());
            if (ret != 1) {
                fclose(file);
                printf("Parsing error in file %s (newmtl)\n", file_name);
                return false;
            }
            m_name.resize(strlen(m_name.data()));

            /*
            Syntax example:

            newmtl Material.022
            Ns 225.000000
            Ka 1.000000 1.000000 1.000000
            Kd 0.800000 0.800000 0.800000
            Ks 0.500000 0.500000 0.500000
            Ke 0.000000 0.000000 0.000000
            Ni 1.450000
            d 1.000000
            illum 2
            map_Kd file_name.png
            */
            double ns;
            ret = fscanf(file, "Ns %lf\n", &ns);
            if (ret != 1) {
                fclose(file);
                printf("Parsing error in file %s (Ns)\n", file_name);
                return false;
            }

            double ka_r, ka_g, ka_b;
            ret = fscanf(file, "Ka %lf %lf %lf\n", &ka_r, &ka_g, &ka_b);
            if (ret != 3) {
                fclose(file);
                printf("Parsing error in file %s (Ka)\n", file_name);
                return false;
            }

            double kd_r, kd_g, kd_b;
            ret = fscanf(file, "Ka %lf %lf %lf\n", &kd_r, &kd_g, &kd_b);
            if (ret != 3) {
                fclose(file);
                printf("Parsing error in file %s (Kd)\n", file_name);
                return false;
            }

            double ks_r, ks_g, ks_b;
            ret = fscanf(file, "Ka %lf %lf %lf\n", &ks_r, &ks_g, &ks_b);
            if (ret != 3) {
                fclose(file);
                printf("Parsing error in file %s (Ks)\n", file_name);
                return false;
            }

            double ke_r, ke_g, ke_b;
            ret = fscanf(file, "Ka %lf %lf %lf\n", &ke_r, &ke_g, &ke_b);
            if (ret != 3) {
                fclose(file);
                printf("Parsing error in file %s (Ke)\n", file_name);
                return false;
            }

            double ni;
            ret = fscanf(file, "Ni %lf\n", &ni);
            if (ret != 1) {
                fclose(file);
                printf("Parsing error in file %s (Ni)\n", file_name);
                return false;
            }

            double d;
            ret = fscanf(file, "d %lf\n", &d);
            if (ret != 1) {
                fclose(file);
                printf("Parsing error in file %s (d)\n", file_name);
                return false;
            }

            unsigned int illum;
            ret = fscanf(file, "illum %u\n", &d);
            if (ret != 1) {
                fclose(file);
                printf("Parsing error in file %s (illum)\n", file_name);
                return false;
            }

            char tfile_name[513];
            ret = fscanf(file, "map_Ka %512s\n");
            //if (ret == 0) {
                // map_Ka omitted
                const material m(ns, rt::color(ka_r, ka_g, ka_b), rt::color(kd_r, kd_g, kd_b),
                    rt::color(ks_r, ks_g, ks_b), rt::color(ke_r, ke_g, ke_b),
                    ni, d, illum);
                material_names.push_back(m_name);
                material_set.push_back(m);
            // }
            // else {
            //     // I need to find a way to link the material with a texture...
            // }
        }

    }

    fclose(file);
    return true;
}
