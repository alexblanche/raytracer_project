#include "file_readers/mtl_parser.hpp"

#include "scene/material/material.hpp"
#include "scene/material/texture.hpp"
#include "file_readers/file.hpp"

#include <vector>
#include <string>
#include <stdexcept>

/* Mtl file parser */

/* Defines the new materials from the mtl file file_name,
   places their name in material_names, and places the materials in material_set

    When a texture is loaded with map_Ka / Kd, it is placed in texture_set (like in the scene parser)
      and the association table is updated with a new pair (m_index, t_index) so that each material created
      with index m_index must have the texture t_index

   Returns true if the operation was successful */

static inline void check(exit_status status, const std::string& error_message) {
    if (status == exit_status::Failure) {
        throw std::runtime_error(error_message + "\n");
    }
}

exit_status parse_mtl_file(const std::string& file_name, const std::string& path,
    std::vector<wrapper<material>>& material_wrapper_set,
    std::vector<wrapper<texture>>& texture_wrapper_set,
    std::map<unsigned int, unsigned int>& mt_assoc, const real gamma) {

    file f(path + file_name);

    try {

        /* Parsing loop */
        while (not f.eof()) {

            // longest items are newmtl
            char buffer[7];
            const exit_status status = f.read<char>(buffer);
            if (status == exit_status::Failure)
                break;
            
            const std::string s(buffer);

            /* Commented line, or ignored command */
            if (s == "#") {
                f.skip_line();
            }
            else if (s == "newmtl") {
                /* Generating a new material */
                std::string m_name = f.read_string(64);

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
                check(f.scanf("Ns %lf\n", ns), "(Ns)");

                double ka_r, ka_g, ka_b;
                check(f.scanf("Ka %lf %lf %lf\n", ka_r, ka_g, ka_b), "(Ka)");

                double kd_r, kd_g, kd_b;
                check(f.scanf("Kd %lf %lf %lf\n", kd_r, kd_g, kd_b), "(Kd)");

                double ks_r, ks_g, ks_b;
                check(f.scanf("Ks %lf %lf %lf\n", ks_r, ks_g, ks_b), "(Ks)");

                double ke_r, ke_g, ke_b;
                check(f.scanf("Ke %lf %lf %lf\n", ke_r, ke_g, ke_b), "(Ke)");

                double ni;
                check(f.scanf("Ni %lf\n", ni), "(Ni)");

                double d;
                check(f.scanf("d %lf\n", d), "(d)");

                unsigned int illum;
                check(f.scanf("illum %u\n", illum), "(illum)");

                /* Looking for a material with the same name */
                bool already_exists = false;
                constexpr bool SILENT = true;
                std::optional<unsigned int> v_opt = wrapper<material>::find_element(material_wrapper_set, m_name, SILENT);
                if (v_opt.has_value()) {
                    printf("\rDuplicate material %s ignored\n", m_name.c_str());
                    already_exists = true;
                }

                if (not already_exists) {
                    material m(ns, rt::color(ka_r, ka_g, ka_b), rt::color(kd_r, kd_g, kd_b),
                        rt::color(ks_r, ks_g, ks_b), rt::color(ke_r, ke_g, ke_b),
                        ni, d, illum, gamma);
                    material_wrapper_set.emplace_back(std::move(m), m_name);
                }
                
                const unsigned int m_i = material_wrapper_set.back().index;

                /* Test for associated texture */
                char c;
                const exit_status status = f.scanf("map_K%c", c);
                const std::string tfile_name = f.read_string(512);
                if (status == exit_status::Success && not already_exists) {
                    // Texture loading

                    bool parsing_successful;
                    texture txt(path + tfile_name, parsing_successful);
                    texture_wrapper_set.emplace_back(std::move(txt));
                    const unsigned int t_i = texture_wrapper_set.back().index;

                    if (parsing_successful) {
                        printf("\rmtl_parser: %s texture loaded\n", tfile_name.c_str());
                    }
                    else {
                        printf("mtl_parser: %s texture reading failed\n", tfile_name.c_str());
                        throw std::runtime_error("(texture reading)");
                    }

                    mt_assoc[m_i] = t_i;
                }
                // else: texture omitted
            }
        }

        return exit_status::Success;

    }
    catch(const std::exception& e) {
        printf("Parsing error in file %s ", file_name.c_str());
        printf("%s\n", e.what());
        return exit_status::Failure;
    }
}
