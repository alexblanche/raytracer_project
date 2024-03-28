#include "file_readers/mtl_parser.hpp"

#include "scene/material/material.hpp"
#include "scene/material/texture.hpp"

#include <vector>
#include <stdio.h>
#include <string.h>
#include <string>

/* Mtl file parser */

/* Defines the new materials from the mtl file file_name,
   places their name in material_names, and places the materials in material_set

    - When a texture is loaded with map_Ka / Kd, it is placed in texture_set (like in the scene parser)
      and the association table is updated with a new pair (m_index, t_index) so that each material created
      with index m_index must have the texture t_index
    - Textures are loaded each time without checking for duplicates, because I assume that it does not happen often
      (otherwise I need a table to remember the already loaded texture's file names)

   Returns true if the operation was successful */

bool parse_mtl_file(const char* file_name, const std::string& path,
    std::vector<std::string>& material_names, std::vector<material>& material_set,
    std::vector<std::string>& texture_names, std::vector<texture>& texture_set,
    mt_assoc& assoc) {

    FILE* file = fopen((path + std::string(file_name)).data(), "r");

    if (file == NULL) {
        printf("Error, file %s not found\n", file_name);
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
            int ret = fscanf(file, " %64s\n", (char*) m_name.data());
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
            ret = fscanf(file, "Kd %lf %lf %lf\n", &kd_r, &kd_g, &kd_b);
            if (ret != 3) {
                fclose(file);
                printf("Parsing error in file %s (Kd)\n", file_name);
                return false;
            }

            double ks_r, ks_g, ks_b;
            ret = fscanf(file, "Ks %lf %lf %lf\n", &ks_r, &ks_g, &ks_b);
            if (ret != 3) {
                fclose(file);
                printf("Parsing error in file %s (Ks)\n", file_name);
                return false;
            }

            double ke_r, ke_g, ke_b;
            ret = fscanf(file, "Ke %lf %lf %lf\n", &ke_r, &ke_g, &ke_b);
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
            ret = fscanf(file, "illum %u\n", &illum);
            if (ret != 1) {
                fclose(file);
                printf("Parsing error in file %s (illum)\n", file_name);
                return false;
            }

            const material m(ns, rt::color(ka_r, ka_g, ka_b), rt::color(kd_r, kd_g, kd_b),
                    rt::color(ks_r, ks_g, ks_b), rt::color(ke_r, ke_g, ke_b),
                    ni, d, illum);
            material_names.push_back(m_name);
            const unsigned int material_index = material_set.size();
            material_set.push_back(m);

            /* Test for associated texture */
            char tfile_name[513];
            char c;
            ret = fscanf(file, "map_K%c %512s\n", &c, tfile_name);
            if (ret == 2) {
                // Texture loading
                const unsigned int texture_index = texture_set.size();
                
                bool parsing_successful;
                texture_set.push_back(
                    texture((path + std::string(tfile_name)).data(), parsing_successful)
                );
                texture_names.push_back("@@@rt_unnamed_texture@@@");

                if (parsing_successful) {
                    printf("\rmtl_parser: %s texture loaded\n", tfile_name);
                    fflush(stdout);
                }
                else {
                    fclose(file);
                    printf("mtl_parser: %s texture reading failed\n", tfile_name);
                    return false;
                }

                assoc.push(material_index, texture_index);
            }
            // else: texture omitted
        }
    }

    fclose(file);
    return true;
}
