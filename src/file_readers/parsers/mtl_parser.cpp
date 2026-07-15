#include "file_readers/parsers/mtl_parser.hpp"

#include "scene/material/material.hpp"
#include "scene/material/texture.hpp"
#include "file_readers/file.hpp"

#include <stdexcept>

static constexpr unsigned int MAX_NAME_LENGTH     = 64;
static constexpr unsigned int MAX_FILENAME_LENGTH = 512;

/* Mtl file parser */

/* Defines the new materials from the mtl file file_name,
   places their name in material_names, and places the materials in material_set

    When a texture is loaded with map_Ka / Kd, it is placed in texture_set (like in the scene parser)
      and the association table is updated with a new pair (m_index, t_index) so that each material created
      with index m_index must have the texture t_index

   Returns true if the operation was successful */

exit_status parse_mtl_file(const std::filesystem::path& path, const std::string& file_name,
    std::vector<wrapper<material>>& material_wrapper_set,
    std::vector<wrapper<texture>>& texture_wrapper_set,
    std::map<unsigned int, unsigned int>& mt_assoc, std::optional<real> gamma) {

    file f((path / file_name).generic_string(), "rb");

    try {

        /* Parsing loop */
        while (not f.eof()) {

            // longest items are newmtl
            const std::string arg = f.read_string(6);

            if (arg.length() == 0)
                break;
            
            /* Commented line, or ignored command */
            if (arg.starts_with("#")) {
                f.skip_line();
                continue;
            }

            if (arg == "newmtl") {

                /* Generating a new material */
                const std::string m_name = f.read_string(MAX_NAME_LENGTH);

                /*
                Syntax examples:

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


                newmtl Material.002
                Ka 1.000000 1.000000 1.000000
                Ks 0.500000 0.500000 0.500000
                Ke 0.000000 0.000000 0.000000
                Ni 1.450000
                d 1.000000
                illum 2
                map_Kd CliffRock_0014_2K_Albedo.bmp
                map_Ns CliffRock_0014_2K_Roughness.bmp
                map_Bump -bm 0.300000 CliffRock_0014_2K_Normal.bmp
                */
                
                double ns = 0.0;
                f.scanf_rewind_if_failure("Ns %lf\n", ns);

                double ka_r = 0.0, ka_g = 0.0, ka_b = 0.0;
                f.scanf_rewind_if_failure("Ka %lf %lf %lf\n", ka_r, ka_g, ka_b);

                double kd_r = 0.0, kd_g = 0.0, kd_b = 0.0;
                f.scanf_rewind_if_failure("Kd %lf %lf %lf\n", kd_r, kd_g, kd_b);

                double ks_r = 0.0, ks_g = 0.0, ks_b = 0.0;
                f.scanf_rewind_if_failure("Ks %lf %lf %lf\n", ks_r, ks_g, ks_b);

                double ke_r = 0.0, ke_g = 0.0, ke_b = 0.0;
                f.scanf_rewind_if_failure("Ke %lf %lf %lf\n", ke_r, ke_g, ke_b);

                double ni = 0.0;
                f.scanf_rewind_if_failure("Ni %lf\n", ni);

                double d = 0.0;
                f.scanf_rewind_if_failure("d %lf\n", d);

                unsigned int illum = 0;
                f.scanf_rewind_if_failure("illum %u\n", illum);

                /* Looking for a material with the same name */
                bool already_exists = false;
                constexpr bool SILENT = true;
                const std::optional<unsigned int> v_opt = wrapper<material>::find_element(material_wrapper_set, m_name, SILENT);
                if (v_opt.has_value()) {
                    printf("\rDuplicate material %s ignored\n", m_name.c_str());
                    already_exists = true;
                }

                if (not already_exists) {

                    material_wrapper_set.emplace_back(
                        material(ns,
                            rt::color(ka_r, ka_g, ka_b),
                            rt::color(kd_r, kd_g, kd_b),
                            rt::color(ks_r, ks_g, ks_b),
                            rt::color(ke_r, ke_g, ke_b),
                            ni, d, illum, gamma
                        ),

                        m_name);
                }

                const unsigned int m_i = material_wrapper_set.back().index;

                /* Test for associated texture */
                char c;
                const exit_status status = f.scanf_rewind_if_failure("map_K%c", c);
                if (status == exit_status::Success && not already_exists) {

                    const std::string tfile_name = f.read_string(MAX_FILENAME_LENGTH);

                    // Texture loading
                    bool parsing_successful;
                    const std::string full_name = (path / tfile_name).generic_string();
                    texture_wrapper_set.emplace_back(texture(full_name, parsing_successful, gamma));
                    mt_assoc[m_i] = texture_wrapper_set.back().index;

                    if (not parsing_successful)
                        throw std::runtime_error(("mtl_parser: " + tfile_name + " texture reading failed\n").c_str());
                    
                    printf("\rmtl_parser: %s texture loaded\n", tfile_name.c_str());
                }
                // else: texture omitted

                // Other maps not yet implemented
                while ((not f.eof()) && exit_status::Success == f.scanf_rewind_if_failure("map_")) {
                    f.skip_line();
                }

                continue;
            }

            throw std::runtime_error(("unexpected keyword " + arg).c_str());
        }

        return exit_status::Success;
    }
    catch(file::error) {
        printf("File error\n");
    }
    catch(const std::exception& e) {
        printf("Parsing error in file %s: ", file_name.c_str());
        printf("%s\n", e.what());
    }
    return exit_status::Failure;
}
