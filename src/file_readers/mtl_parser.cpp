#include "file_readers/mtl_parser.hpp"

#include "scene/material/material.hpp"
#include "scene/material/texture.hpp"

#include <vector>
#include <stdio.h>
#include <string.h>
#include <string>

#include <stdexcept>

/* Mtl file parser */

/* Defines the new materials from the mtl file file_name,
   places their name in material_names, and places the materials in material_set

    When a texture is loaded with map_Ka / Kd, it is placed in texture_set (like in the scene parser)
      and the association table is updated with a new pair (m_index, t_index) so that each material created
      with index m_index must have the texture t_index

   Returns true if the operation was successful */

bool parse_mtl_file(const char* file_name, const std::string& path,
    std::vector<wrapper<material>>& material_wrapper_set,
    std::vector<wrapper<texture>>& texture_wrapper_set,
    std::map<size_t, size_t>& mt_assoc, const real gamma) {

    FILE* file = fopen((path + std::string(file_name)).data(), "r");

    if (file == NULL) {
        printf("Error, file %s not found\n", file_name);
        return false;
    }

    try {

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
                    throw std::runtime_error("(newmtl)");
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
                    throw std::runtime_error("(Ns)\n");
                }

                double ka_r, ka_g, ka_b;
                ret = fscanf(file, "Ka %lf %lf %lf\n", &ka_r, &ka_g, &ka_b);
                if (ret != 3) {
                    throw std::runtime_error("(Ka)");
                }

                double kd_r, kd_g, kd_b;
                ret = fscanf(file, "Kd %lf %lf %lf\n", &kd_r, &kd_g, &kd_b);
                if (ret != 3) {
                    throw std::runtime_error("(Kd)");
                }

                double ks_r, ks_g, ks_b;
                ret = fscanf(file, "Ks %lf %lf %lf\n", &ks_r, &ks_g, &ks_b);
                if (ret != 3) {
                    throw std::runtime_error("(Ks)");
                }

                double ke_r, ke_g, ke_b;
                ret = fscanf(file, "Ke %lf %lf %lf\n", &ke_r, &ke_g, &ke_b);
                if (ret != 3) {
                    throw std::runtime_error("(Ke)");
                }

                double ni;
                ret = fscanf(file, "Ni %lf\n", &ni);
                if (ret != 1) {
                    throw std::runtime_error("(Ni)");
                }

                double d;
                ret = fscanf(file, "d %lf\n", &d);
                if (ret != 1) {
                    throw std::runtime_error("(d)");
                }

                unsigned int illum;
                ret = fscanf(file, "illum %u\n", &illum);
                if (ret != 1) {
                    throw std::runtime_error("(illum)");
                }

                /* Looking for a material with the same name */
                bool already_exists = false;
                for(wrapper<material> const& mat_wrap : material_wrapper_set) {
                    if (mat_wrap.name.has_value() && mat_wrap.name.value().compare(m_name) == 0) {
                        printf("\rDuplicate material %s ignored\n", m_name.data());
                        already_exists = true;
                        break;
                    }
                }

                if (not already_exists) {
                    material m(ns, rt::color(ka_r, ka_g, ka_b), rt::color(kd_r, kd_g, kd_b),
                        rt::color(ks_r, ks_g, ks_b), rt::color(ke_r, ke_g, ke_b),
                        ni, d, illum, gamma);
                    material_wrapper_set.emplace_back(std::move(m), m_name);
                }
                
                const size_t m_i = material_wrapper_set[material_wrapper_set.size()-1].index;

                /* Test for associated texture */
                char tfile_name[513];
                char c;
                ret = fscanf(file, "map_K%c %512s\n", &c, tfile_name);
                if (ret == 2 && not already_exists) {
                    // Texture loading

                    bool parsing_successful;
                    texture txt((path + std::string(tfile_name)).data(), parsing_successful);
                    texture_wrapper_set.emplace_back(std::move(txt));
                    const size_t t_i = texture_wrapper_set[texture_wrapper_set.size()-1].index;

                    if (parsing_successful) {
                        printf("\rmtl_parser: %s texture loaded\n", tfile_name);
                        fflush(stdout);
                    }
                    else {
                        printf("mtl_parser: %s texture reading failed\n", tfile_name);
                        throw std::runtime_error("(texture reading)");
                    }

                    mt_assoc[m_i] = t_i;
                }
                // else: texture omitted
            }
        }

        fclose(file);
        return true;

    }
    catch(const std::exception& e) {
        printf("Parsing error in file %s ", file_name);
        printf("%s\n", e.what());
        fclose(file);
        return false;
    }
}
