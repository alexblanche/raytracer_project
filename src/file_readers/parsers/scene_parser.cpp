#include "file_readers/parsers/scene_parser.hpp"

#include "file_readers/parsers/obj_parser.hpp"

#include "file_readers/file.hpp"
#include "auxiliary/utils.hpp"
// #include "auxiliary/timer.hpp"

#include <string>
#include <sstream>
#include <filesystem>
#include <stdexcept>

static constexpr unsigned int MAX_NAME_LENGTH     = 64;
static constexpr unsigned int MAX_FILENAME_LENGTH = 512;

/*** Scene descriptor pre-parsing ***/
[[maybe_unused]] static pre_parsing_info pre_parse(const file& f) {

    // timer_ms timer;
    // timer.start();

    pre_parsing_info ppi;

    constexpr std::array<std::string, 8> keywords_array = {
        "triangle", "quad", "sphere", "plane", "box", "cylinder",
        "material", "load_texture"
    };
    const std::span keywords(keywords_array);

    std::string arg;
    while (not f.eof()) {
        arg = f.read_string();
        
        std::optional<unsigned int> obj_index_opt = index_of<std::string, 8>(arg, keywords);

        if (obj_index_opt.has_value()) {
            const unsigned int index = obj_index_opt.value();
            switch (index) {
                case 0: ppi.triangles++; break;
                case 1: ppi.quads++;     break;
                case 2: ppi.spheres++;   break;
                case 3: ppi.planes++;    break;
                case 4: ppi.boxes++;     break;
                case 5: ppi.cylinders++; break;
                case 6: ppi.materials++; break;
                case 7: ppi.textures++;  break;
            }
            if (index < 6)
                ppi.objects++;
        }
        else if (arg == "load_obj") {
            const std::string filename = f.read_string(MAX_FILENAME_LENGTH);
            const auto& [ _, obj_triangles, obj_quads ] = pre_parse_obj(filename);
            ppi.triangles += obj_triangles;
            ppi.quads     += obj_quads;
            ppi.objects   += obj_triangles + obj_quads;
        }

        f.skip_line();
    }
    
    /*
    std::cout << "pre_parse_info:"
        << "\ntriangles: " << ppi.triangles
        << "\nquads:     " << ppi.quads
        << "\nspheres:   " << ppi.spheres
        << "\nplanes:    " << ppi.planes
        << "\nboxes:     " << ppi.boxes
        << "\ncylinders: " << ppi.cylinders
        << "\nmaterials: " << ppi.materials
        << "\ntextures:  " << ppi.textures  << std::endl;
    */

    f.rewind();

    // timer.stop();
    // timer.print();
    
    return ppi;
}

/*** Scene description parsing ***/

/* Auxiliary function: returns a material from a description file */
static std::optional<material> parse_material(const file& f, const real gamma) {
    /* color:(120, 120, 120) emitted_color:(0, 0, 0) reflectivity:1 emission:0
        specular_p:1.0 reflects_color:false transparency:0.5 scattering:0 refraction_index:1.2)

        See the file structure below.    
    */

    struct mat_properties {
        double r, g, b, smooth, em_int, refl, transp, scattering, refr_i;
        bool refl_color;
    };

    constexpr auto copy_material = [] (const material& mat) {
        return mat_properties {
            .r            = mat.get_color().red,
            .g            = mat.get_color().green,
            .b            = mat.get_color().blue,
            .smooth       = mat.get_smoothness(),
            .em_int       = mat.get_emission_intensity(),
            .refl         = mat.get_reflectivity(),
            .transp       = mat.get_transparency(),
            .scattering   = mat.get_refraction_scattering(),
            .refr_i       = mat.get_refraction_index(),
            .refl_color   = mat.does_reflect_color()
        };
    };
    mat_properties mp = copy_material(DIFFUSE);

    constexpr int BUFFER_MAX_SIZE = 256;
    char buffer[BUFFER_MAX_SIZE];
    // Manual extraction (to avoid going over texture declaration)
    f.skip_whitespace();
    f.skip_char('(', 1);
    int depth = 0;
    int i;
    for (i = 0; i < BUFFER_MAX_SIZE; i++) {
        const char c = f.getc();
        if (c == '(')
            depth++;
        else if (c == ')') {
            if (depth == 0) {
                buffer[i] = '\0';
                break;
            }
            depth--;
        }
        buffer[i] = c;
    }
    
    if (i == BUFFER_MAX_SIZE) {
        printf("parsing error in parse_material: material definition is too long\n");
        return std::nullopt;
    }

    struct param_set {
        bool color      = false;
        bool smooth     = false;
        bool em_int     = false;
        bool refl       = false;
        bool transp     = false;
        bool scattering = false;
        bool refr_i     = false;
        bool refl_col   = false;
        int  nb_param   = 0;
    };
    param_set is_set;

    std::istringstream stream(buffer + (buffer[0] == '('));
    std::string word;
    while (stream >> word) {
        if (is_set.nb_param >= 9) {
            printf("parsing error in parse_material: too many parameters set\n");
            return std::nullopt;
        }

        if (word.starts_with("diffuse")) {
            if (is_set.nb_param) {
                printf("parsing error in parse_material: no parameter should be set in addition to diffuse\n");
                return std::nullopt;
            }
            // Default is diffuse white
            break;
        }
        else if (word.starts_with("mirror")) {
            if (is_set.nb_param) {
                printf("parsing error in parse_material: no parameter should be set in addition to mirror\n");
                return std::nullopt;
            }
            mp = copy_material(MIRROR);
            break;
        }
        else if (word.starts_with("glass")) {
            if (is_set.nb_param) {
                printf("parsing error in parse_material: no parameter should be set in addition to glass\n");
                return std::nullopt;
            }
            mp = copy_material(GLASS);
            break;
        }
        else if (word.starts_with("water")) {
            if (is_set.nb_param) {
                printf("parsing error in parse_material: no parameter should be set in addition to water\n");
                return std::nullopt;
            }
            mp = copy_material(WATER);
            break;
        }

        if (word.starts_with("color:")) {
            if (is_set.color) {
                printf("parsing error in parse_material: duplicate color definition\n");
                return std::nullopt;
            }
            const int ret1 = sscanf(word.data(), "color:(%lf,%lf,%lf)", &mp.r, &mp.g, &mp.b);
            int ret2 = 0, ret3 = 0;
            if (ret1 == 1) {
                stream >> word;
                ret2 = sscanf(word.data(), "%lf,%lf)", &mp.g, &mp.b);
                if (ret2 == 1) {
                    stream >> word;
                    ret3 = sscanf(word.data(), "%lf)", &mp.b);
                }
            }
            else if (ret1 == 2) {
                stream >> word;
                ret3 = sscanf(word.data(), "%lf)", &mp.b);
            }
            if (ret1 + ret2 + ret3 != 3) {
                printf("parsing error in parse_material: color\n");
                return std::nullopt;
            }
            is_set.color = true;
            is_set.nb_param++;
        }

        /*
        else if (word.starts_with("emitted_color:")) {
            if (is_set.em_color) {
                printf("parsing error in parse_material: duplicate emitted color definition\n");
                return std::nullopt;
            }
            const int ret1 = sscanf(word.data(), "emitted_color:(%lf,%lf,%lf)", &mp.er, &mp.eg, &mp.eb);
            int ret2 = 0, ret3 = 0;
            if (ret1 == 1) {
                stream >> word;
                ret2 = sscanf(word.data(), "%lf,%lf)", &mp.eg, &mp.eb);
                if (ret2 == 1) {
                    stream >> word;
                    ret3 = sscanf(word.data(), "%lf)", &mp.eb);
                }
            }
            else if (ret1 == 2) {
                stream >> word;
                ret3 = sscanf(word.data(), "%lf)", &mp.eb);
            }
            if (ret1 + ret2 + ret3 != 3) {
                // printf("Faulty word: %s\n", word.data());
                printf("parsing error in parse_material: emitted color\n");
                return std::nullopt;
            }
            is_set.em_color = true;
            is_set.nb_param++;
        }
        */

        else if (word.starts_with("smoothness:")) {
            if (is_set.smooth) {
                printf("parsing error in parse_material: duplicate smoothness definition\n");
                return std::nullopt;
            }
            const int ret = sscanf(word.data(), "smoothness:%lf", &mp.smooth);
            if (ret != 1) {
                printf("parsing error in parse_material: smoothness\n");
                return std::nullopt;
            }
            is_set.smooth = true;
            is_set.nb_param++;
        }

        else if (word.starts_with("emission:")) {
            if (is_set.em_int) {
                printf("parsing error in parse_material: duplicate emission definition\n");
                return std::nullopt;
            }
            const int ret = sscanf(word.data(), "emission:%lf", &mp.em_int);
            if (ret != 1) {
                printf("parsing error in parse_material: emission\n");
                return std::nullopt;
            }
            is_set.em_int = true;
            is_set.nb_param++;
        }

        else if (word.starts_with("reflectivity:")) {
            if (is_set.refl) {
                printf("parsing error in parse_material: duplicate reflectivity definition\n");
                return std::nullopt;
            }
            const int ret = sscanf(word.data(), "reflectivity:%lf", &mp.refl);
            if (ret != 1) {
                printf("parsing error in parse_material: reflectivity\n");
                return std::nullopt;
            }
            is_set.refl = true;
            is_set.nb_param++;
        }

        else if (word.starts_with("transparency:")) {
            if (is_set.transp) {
                printf("parsing error in parse_material: duplicate transparency definition\n");
                return std::nullopt;
            }
            const int ret = sscanf(word.data(), "transparency:%lf", &mp.transp);
            if (ret != 1) {
                printf("parsing error in parse_material: transparency\n");
                return std::nullopt;
            }
            is_set.transp = true;
            is_set.nb_param++;
        }

        else if (word.starts_with("scattering:")) {
            if (is_set.scattering) {
                printf("parsing error in parse_material: duplicate scattering definition\n");
                return std::nullopt;
            }
            const int ret = sscanf(word.data(), "scattering:%lf", &mp.scattering);
            if (ret != 1) {
                printf("parsing error in parse_material: scattering\n");
                return std::nullopt;
            }
            is_set.scattering = true;
            is_set.nb_param++;
        }

        else if (word.starts_with("refraction_index:")) {
            if (is_set.refr_i) {
                printf("parsing error in parse_material: duplicate refraction_index definition\n");
                return std::nullopt;
            }
            const int ret = sscanf(word.data(), "refraction_index:%lf", &mp.refr_i);
            if (ret != 1) {
                printf("parsing error in parse_material: refraction_index\n");
                return std::nullopt;
            }
            is_set.refr_i = true;
            is_set.nb_param++;
        }

        else if (word.starts_with("reflects_color:")) {
            if (is_set.refl_col) {
                printf("parsing error in parse_material: duplicate reflects_color definition\n");
                return std::nullopt;
            }
            if (word.starts_with("reflects_color:true")) {
                mp.refl_color = true;
            }
            else if (word.starts_with("reflects_color:false")) {
                mp.refl_color = false;
            }
            else {
                printf("parsing error in parse_material: reflects_color\n");
                return std::nullopt;
            }
            is_set.refl_col = true;
            is_set.nb_param++;
        }

        else if (word.starts_with("texture:") || word.starts_with(")")) {
            break;
        }
        
        else {
            printf("parsing error in parse_material: %s\n", word.data());
            return std::nullopt;
        }
    }

    rt::color mat_color(mp.r, mp.g, mp.b);
    // rt::color em_color(er, eg, eb);
    if (gamma != 1.0_r) {
        mat_color.apply_gamma(gamma);
        // em_color.apply_gamma(gamma);
    }

    return material(mat_color, mp.smooth, mp.em_int, mp.refl, mp.refl_color, mp.transp, mp.scattering, mp.refr_i);
}


/* Auxiliary function: parses the name of a material and returns its index in material_set,
   or parses a new material, stores it in material_set and returns its index */
static std::optional<unsigned int> get_material(const file& f, std::vector<wrapper<material>>& material_wrapper_set, const real gamma) {

    const char firstchar = f.getc();
    
    if (firstchar == '(') {
        // material declaration
        std::optional<material> m = parse_material(f, gamma);

        if (not m.has_value())
            return std::nullopt;
        
        material_wrapper_set.emplace_back(std::move(m.value()));
        return material_wrapper_set.back().index;
    }
    else {
        // Moving back the pointer back by one position
        f.ungetc(firstchar);

        // material variable name
        const std::string vname = f.read_string(MAX_NAME_LENGTH);
        return wrapper<material>::find_element(material_wrapper_set, vname);
    }
}

/* Auxiliary function: parses the texture information after a material, and if there is one,
   returns the associated texture_info
   is_triangle is true if the object is a triangle (in this case, 3 uv points are parsed),
   and false if it is a quad (4 uv points are parsed) */
static std::optional<texture_info> parse_texture_info(const file& f,
    const std::vector<wrapper<texture>>& texture_wrapper_set,
    const std::vector<wrapper<normal_map>>& normal_map_wrapper_set,
    // const std::vector<wrapper<roughness_map>>& roughness_map_wrapper_set,
    const object_type object_type) {

    const exit_status status_t = f.scanf_rewind_if_failure("texture");

    if (status_t == exit_status::Failure)
        return std::nullopt;
    
    std::optional<unsigned int> nindex;
    double u0, v0, u1, v1, u2, v2, u3, v3;
    double x0, y0, z0, x1, y1, z1;

    const std::string t_name = f.read_string(MAX_NAME_LENGTH);
    const std::optional<unsigned int> vindex = wrapper<texture>::find_element(texture_wrapper_set, t_name);

    const exit_status status_n = f.scanf_rewind_if_failure("normal:");
    if (status_n == exit_status::Success) {
        const std::string n_name = f.read_string(MAX_NAME_LENGTH);
        nindex = wrapper<normal_map>::find_element(normal_map_wrapper_set, n_name); 
    }
    // else: no normal map information

    /*
    // Roughness map

    std::optional<unsigned int> rindex;
    const std::size_t pos3 = f.position();
    const std::string keyword_r = f.read_string(6);
    if (keyword_r != "rough:") {
        // No roughness info, setting the position back before the keyword
        f.rewind(pos3);
    }
    else {
        const std::string r_name = f.read_string(MAX_NAME_LENGTH);
        rindex = wrapper<roughness_map>::find_element(roughness_map_wrapper_set, r_name);
    }
    */
    
    using enum object_type;
    switch (object_type) {
        case Triangle: {

            const exit_status status = f.scanf(" (%lf,%lf) (%lf,%lf) (%lf,%lf))\n",
                u0, v0, u1, v1, u2, v2);
            if (status == exit_status::Failure) {
                printf("parsing error in parse_texture_info (triangle UV-coordinates)\n");
                return std::nullopt;
            }

            return texture_info(vindex, nindex, { u0, v0, u1, v1, u2, v2 });
        }
                    
        case Quad: {

            const exit_status status = f.scanf(" (%lf,%lf) (%lf,%lf) (%lf,%lf) (%lf,%lf))\n",
                u0, v0, u1, v1, u2, v2, u3, v3);
            if (status == exit_status::Failure) {
                printf("parsing error in parse_texture_info (quad UV-coordinates)\n");
                return std::nullopt;
            }

            return texture_info(vindex, nindex, { u0, v0, u1, v1, u2, v2, u3, v3 });
        }

        case Sphere: {

            const exit_status status = f.scanf(" forward:(%lf,%lf,%lf) right:(%lf,%lf,%lf))\n",
                x0, y0, z0, x1, y1, z1);
            if (status == exit_status::Failure) {
                printf("parsing error in parse_texture_info (sphere forward and right directions)\n");
                return std::nullopt;
            }

            // texture_info is used to pass the coordinates for forward_dir and right_dir
            return texture_info(vindex, nindex, { x0, y0, z0, x1, y1, z1 });
        }

        case Plane: {

            const exit_status status = f.scanf(" right:(%lf,%lf,%lf) scale:%lf)\n",
                x0, y0, z0, u0);
            if (status == exit_status::Failure) {
                printf("parsing error in parse_texture_info (plane right direction and scale)\n");
                return std::nullopt;
            }

            // texture_info is used to pass the coordinates for forward_dir and right_dir
            return texture_info(vindex, nindex, { x0, y0, z0, u0 });
        }

        case Box:      throw std::runtime_error("box texturing not handled yet");
        case Cylinder: throw std::runtime_error("cylinder texturing not handled yet");
        default:       throw std::runtime_error("incorrect object type");
    }
}

static void parse_objects(const file& f, const object_type type, const std::string& arg,
        containers& containers, const bool bounding_enabled, double inverse_gamma) {
    
    union object_constructor_parameters {
        struct { rt::vector center; real radius; }                    sphere;
        struct { rt::vector position, normal; }                       plane;
        struct { rt::vector center, x_axis, y_axis; real l[3]; }      box;
        struct { rt::vector p[3]; }                                   triangle;
        struct { rt::vector p[4]; }                                   quad;
        struct { rt::vector origin, direction; real radius, length; } cylinder;

        object_constructor_parameters() {}
    };
    object_constructor_parameters parameters;
    exit_status status;

    using enum object_type;
    switch (type) {

        case Sphere: {

            /* center:(-500, 0, 600) radius:120 [material] */
            double posx, posy, posz, r;
            status = f.scanf("center:(%lf,%lf,%lf) radius:%lf material:", posx, posy, posz, r);
            
            parameters.sphere = {
                .center = rt::vector(posx, posy, posz),
                .radius = r
            };
            break;
        }

        case Plane: {

            /* normal:(0, -1, 0) position:(0, 160, 0) [material] */
            double nx, ny, nz, px, py, pz;
            status = f.scanf("normal:(%lf,%lf,%lf) position:(%lf,%lf,%lf) material:",
                nx, ny, nz, px, py, pz);
            
            parameters.plane = {
                .position = rt::vector(px, py, pz),
                .normal   = rt::vector(nx, ny, nz)
            };
            break;
        }

        case Box: {

            /* center:(166, -200, 600) x_axis:(100, 100, -100) y_axis:(-200, 100, -100) 300 200 300 */
            double cx, cy, cz, n1x, n1y, n1z, n2x, n2y, n2z, lx, ly, lz;
            status = f.scanf("center:(%lf,%lf,%lf) x_axis:(%lf,%lf,%lf) y_axis:(%lf,%lf,%lf) %lf %lf %lf material:",
                cx, cy, cz, n1x, n1y, n1z, n2x, n2y, n2z, lx, ly, lz);
            
            parameters.box = {
                .center = rt::vector(cx, cy, cz),
                .x_axis = rt::vector(n1x, n1y, n1z).unit(),
                .y_axis = rt::vector(n2x, n2y, n2z).unit(),
                .l      = { lx, ly, lz }
            };
            break;
        }

        case Triangle: {

            /* (-620, -100, 600) (-520, 100, 500) (-540, -200, 700) [material] */
            double v0x, v0y, v0z, v1x, v1y, v1z, v2x, v2y, v2z;
            status = f.scanf("(%lf,%lf,%lf) (%lf,%lf,%lf) (%lf,%lf,%lf) material:",
                v0x, v0y, v0z, v1x, v1y, v1z, v2x, v2y, v2z);
            
            parameters.triangle = {
                .p = {
                    rt::vector(v0x, v0y, v0z),
                    rt::vector(v1x, v1y, v1z),
                    rt::vector(v2x, v2y, v2z)
                }
            };
            break;
        }

        case Quad: {

            /* (-620, -100, 600) (-520, 100, 600) (-540, -200, 600) (-500, -250, 600) [material] */
            double v0x, v0y, v0z, v1x, v1y, v1z, v2x, v2y, v2z, v3x, v3y, v3z;
            status = f.scanf("(%lf,%lf,%lf) (%lf,%lf,%lf) (%lf,%lf,%lf) (%lf,%lf,%lf) material:",
                v0x, v0y, v0z, v1x, v1y, v1z, v2x, v2y, v2z, v3x, v3y, v3z);
            
            parameters.quad = {
                .p = {
                    rt::vector(v0x, v0y, v0z),
                    rt::vector(v1x, v1y, v1z),
                    rt::vector(v2x, v2y, v2z),
                    rt::vector(v3x, v3y, v3z)
                }
            };
            break;
        }

        case Cylinder: {

            /* origin:(0, 0, 0) direction:(1, -1, 1) radius:100 length:300 [material] */
            double px, py, pz, d_x, d_y, d_z, r, l;
            status = f.scanf("origin:(%lf,%lf,%lf) direction:(%lf,%lf,%lf) radius:%lf length:%lf material:",
                px, py, pz, d_x, d_y, d_z, r, l);
            
            parameters.cylinder = {
                .origin    = rt::vector(px, py, pz),
                .direction = rt::vector(d_x, d_y, d_z).unit(),
                .radius    = r,
                .length    = l
            };
            break;
        }
    }

    throw_if_failure(status, "parsing error in scene constructor (" + arg + " declaration)");

    auto& [
        object_set,
        other_content,

        triangle_set,
        quad_set,
        sphere_set,
        plane_set,
        box_set,
        cylinder_set,

        material_wrapper_set,
        texture_wrapper_set,
        normal_map_wrapper_set,
        texture_info_set
    ]
    = containers;

    const std::optional<unsigned int> m_index = get_material(f, material_wrapper_set, inverse_gamma);
    throw_if_null(m_index, "material definition error");

    const object* obj = nullptr;

    if (belongs_to(type, { Box, Cylinder })) {
        switch (type) {
            case Box: {
                const auto& [ center, x_axis, y_axis, l ] = parameters.box;
                const auto [ lx, ly, lz ] = l;
                box_set.emplace_back(center, x_axis, y_axis, lx, ly, lz, m_index.value());
                obj = &box_set.back();
                break;
            }
            case Cylinder: {
                const auto& [ origin, direction, radius, length ] = parameters.cylinder;
                cylinder_set.emplace_back(origin, direction, radius, length, m_index.value());
                obj = &cylinder_set.back();
                break;
            }
            default: throw;
        }
    }
    else {
        std::optional<texture_info> info = parse_texture_info(f, texture_wrapper_set, normal_map_wrapper_set, type);
    
        if (info.has_value()) {

            const bool normal_mapping = info.value().has_normal_information();

            switch (type) {

                case Sphere: {
                    const auto& [ fx, fy, fz, rx, ry, rz, _, _ ] = info.value().uv_coordinates;
                    const rt::vector forward(fx, fy, fz);
                    const rt::vector right  (rx, ry, rz);
                    const auto& [ center, radius ] = parameters.sphere;
                    sphere_set.emplace_back(center, radius, m_index.value(), texture_info_set.size(), forward, right);
                    obj = &sphere_set.back();
                    break;
                }

                case Plane: {
                    const auto& [ rx, ry, rz, scale, _, _, _, _ ] = info.value().uv_coordinates;
                    const rt::vector right(rx, ry, rz);
                    const auto& [ position, normal ] = parameters.plane;
                    plane_set.emplace_back(normal.x, normal.y, normal.z, position, m_index.value(), texture_info_set.size(), right, scale);
                    obj = &plane_set.back();
                    break;
                }

                case Triangle: {
                    const auto& [ v ] = parameters.triangle;
                    if (normal_mapping)
                        triangle_set.emplace_back(v[0], v[1], v[2], m_index.value(), texture_info_set.size(), info.value());
                    else
                        triangle_set.emplace_back(v[0], v[1], v[2], m_index.value(), texture_info_set.size());
                    obj = &triangle_set.back();
                    break;
                }

                case Quad: {
                    const auto& [ v ] = parameters.quad;
                    if (normal_mapping)
                        quad_set.emplace_back(v[0], v[1], v[2], v[3], m_index.value(), texture_info_set.size(), info.value());
                    else
                        quad_set.emplace_back(v[0], v[1], v[2], v[3], m_index.value(), texture_info_set.size());
                    obj = &quad_set.back();
                    break;
                }

                default:
                    throw;
            }

            texture_info_set.emplace_back(std::move(info.value()));
        }
        else {

            switch (type) {

                case Sphere: {
                    const auto& [ center, radius ] = parameters.sphere;
                    //obj = new sphere(center, radius, m_index.value());
                    sphere_set.emplace_back(center, radius, m_index.value());
                    obj = &sphere_set.back();
                    break;
                }
                
                case Plane: {
                    const auto& [ position, normal ] = parameters.plane;
                    plane_set.emplace_back(normal.x, normal.y, normal.z, position, m_index.value());
                    obj = &plane_set.back();
                    break;
                }

                case Triangle: {
                    const auto& [ v ] = parameters.triangle;
                    triangle_set.emplace_back(v[0], v[1], v[2], m_index.value());
                    obj = &triangle_set.back();
                    break;
                }

                case Quad: {
                    const auto& [ v ] = parameters.quad;
                    quad_set.emplace_back(v[0], v[1], v[2], v[3], m_index.value());
                    obj = &quad_set.back();
                    break;
                }

                default:
                    break;
            }
        }
    }

    object_set.push_back(obj);

    if (bounding_enabled)
        other_content.push_back(obj);
}


/** Scene description parser **/

std::optional<scene> parse_scene_descriptor(const std::string& file_name) {

    timer_ms timer;
    timer.start();

    std::optional<scene> scene_opt;

    try {

        file f(file_name);

        const pre_parsing_info pre_parsing_info = pre_parse(f);

        /* Parameters definition

        Example:

        resolution width:1366 height:768
        camera position:(0, 0, 0) direction:(0, 0, 1) rightdir:(1, 0, 0) fov_width:1000 distance:400 [focal_distance:500 aperture:100] (optional)
        background_color 190 235 255
        background_texture filename.bmp rotate_x:3.14 rotate_y:5.835 rotate_z:0 gamma:2.2
        polygons_per_bounding 10 //specifying 0 will deactivate the bounding generation

        - At least one of background_color, background_texture must be specified
        - filename.bmp or filename.hdr should designate a panoramic image
        - all angles must be between 0 and 2*pi
        - gamma will be applied to the whole picture, so all the non-background colors (including the textures) are first corrected with the inverse gamma correction. Gamma is optional.

        fov_height is generated automatically (for width/height aspect ratio)
        */
        int width, height;
        double posx, posy, posz, dx, dy, dz, rdx, rdy, rdz, fovw, dist, focl, apr;
        bool depth_of_field_enabled = true;

        {
            const exit_status status1 = f.scanf("resolution width:%d height:%d\n", width, height);
            throw_if_failure(status1, "parsing error in scene constructor (resolution)");
        }

        {
            const int ret = f.scanf_count("camera position:(%lf,%lf,%lf) direction:(%lf,%lf,%lf) rightdir:(%lf,%lf,%lf) fov_width:%lf distance:%lf focal_distance:%lf aperture:%lf\n",
                posx, posy, posz, dx, dy, dz, rdx, rdy, rdz, fovw, dist, focl, apr);
                
            if (ret < 11)
                throw std::runtime_error("parsing error in scene constructor (camera)");
            
            if (ret == 11) // Focal length and aperture omitted
                depth_of_field_enabled = false;
        }

        rt::vector cam_pos(posx, posy, posz);
        rt::vector cam_dir(dx, dy, dz);
        rt::vector cam_right_dir(rdx, rdy, rdz);

        const real fovh = fovw * static_cast<real>(height) / static_cast<real>(width);

        camera cam = depth_of_field_enabled ?
              camera(cam_pos, cam_dir, cam_right_dir, fovw, fovh, dist, width, height, focl, apr)
            : camera(cam_pos, cam_dir, cam_right_dir, fovw, fovh, dist, width, height);

        bool background_texture_is_set = false;
        rt::color background_color;
        texture background_texture;

        // Setting up the background_color or texture
        double r, g, b;
        char bg_tfile_name[513];
        double rx = 0.0, ry = 0.0, rz = 0.0, inverse_gamma = 1.0;
        {
            const exit_status status_background = f.scanf_rewind_if_failure("background_color %lf %lf %lf\n", r, g, b);
            if (status_background == exit_status::Success) {
                background_color = rt::color(r, g, b);
            }
            else {
                const int ret_bg_texture = f.scanf_count("background_texture %512s rotate_x:%lf rotate_y:%lf rotate_z:%lf gamma:%lf\n",
                    bg_tfile_name, rx, ry, rz, inverse_gamma);
                if (ret_bg_texture < 1)
                    throw std::runtime_error("parsing error in scene constructor (background)");
                if (std::abs(rx) > 2.0_r * PI || std::abs(ry) > 2.0_r * PI || std::abs(rz) > 2.0_r * PI)
                    throw std::runtime_error("incorrect background texture angles");
                
                const std::string bg_tfile_name_short = std::filesystem::path(bg_tfile_name).filename().generic_string();

                if (rx < 0) rx += 2.0_r * PI;
                if (ry < 0) ry += 2.0_r * PI;
                if (rz < 0) rz += 2.0_r * PI;

                printf("Parsing %s... ", bg_tfile_name_short.c_str());
                fflush(stdout);

                bool bg_parsing_successful;
                background_texture = texture(bg_tfile_name, bg_parsing_successful);
                if (not bg_parsing_successful)
                    throw std::runtime_error("parsing error in scene constructor (background texture parsing)");
                
                printf("\r> %s texture loaded\n", bg_tfile_name_short.c_str());
                background_texture_is_set = true;
            }
        }
        
        unsigned int polygons_per_bounding = 0;

        // Optional
        {
            f.scanf_rewind_if_failure("bvh: ");
            if (exit_status::Failure == f.scanf("polygons_per_bounding %u\n", polygons_per_bounding)) {
                throw_if_failure(f.scanf("disabled\n"),
                    "parsing error in scene constructor (BVH parameters)");
            }
        }

        std::vector<const object*> object_set;
        object_set.reserve(pre_parsing_info.objects);

        std::vector<triangle> triangle_set;
        std::vector<quad>     quad_set;
        std::vector<sphere>   sphere_set;
        std::vector<plane>    plane_set;
        std::vector<box>      box_set;
        std::vector<cylinder> cylinder_set;
        triangle_set.reserve(pre_parsing_info.triangles + 2 * pre_parsing_info.quads);
        // triangle_set must make room for split quads
        quad_set    .reserve(pre_parsing_info.quads);
        sphere_set  .reserve(pre_parsing_info.spheres);
        plane_set   .reserve(pre_parsing_info.planes);
        box_set     .reserve(pre_parsing_info.boxes);
        cylinder_set.reserve(pre_parsing_info.cylinders);

        /* Material storage */
        std::vector<wrapper<material>> material_wrapper_set;
        material_wrapper_set.emplace_back(DIFFUSE, "diffuse");
        material_wrapper_set.emplace_back(MIRROR,  "mirror" );
        material_wrapper_set.emplace_back(GLASS,   "glass"  );
        material_wrapper_set.emplace_back(WATER,   "water"  );

        std::vector<wrapper<texture>>    texture_wrapper_set;
        std::vector<wrapper<normal_map>> normal_map_wrapper_set;
        
        std::vector<texture_info> texture_info_set;

        std::vector<const bounding*> bounding_set;

        /* Bounding handling */
        /* When bvh is disabled, the objects that are not defined in an obj file are placed in
        the vector other_content. At the end, these objects are placed in a bounding alongside the ones generated during obj files parsing */
        std::vector<const object*> other_content;
        other_content.reserve(
            pre_parsing_info.spheres + pre_parsing_info.planes + pre_parsing_info.boxes + pre_parsing_info.cylinders
        );
        const bool bounding_enabled = polygons_per_bounding != 0;

        containers containers = {
            object_set,
            other_content,
            triangle_set,
            quad_set,
            sphere_set,
            plane_set,
            box_set,
            cylinder_set,
            material_wrapper_set,
            texture_wrapper_set,
            normal_map_wrapper_set,
            texture_info_set
        };

        /* Parsing loop */

        while (not f.eof()) {

            // longest item is load_normal_map, of length 15
            const std::string arg = f.read_string(17);

            if (f.eof())
                break;

            /* Commented line */
            if (arg.starts_with("#")) {
                f.skip_line();
                continue;
            }
            
            /* Material declaration */
            if (arg == "material") {
                const std::string m_name = f.read_string(MAX_NAME_LENGTH);

                std::optional<material> m = parse_material(f, inverse_gamma);
                throw_if_null(m, "material parsing error");
                
                material_wrapper_set.emplace_back(std::move(m.value()), m_name);
                continue;
            }
            
            /* BMP file loading */
            if (belongs_to(arg, { "load_texture", "load_normal_map" })) {

                const bool is_texture = arg == "load_texture";
                const std::string type = is_texture ? "texture" : "normal map";
                
                const std::string t_name = f.read_string(MAX_NAME_LENGTH);
                const std::string tfile_name = f.read_string(MAX_FILENAME_LENGTH);
                throw_if_failure(exit_status_of(t_name.length() != 0 && tfile_name.length() != 0),
                    "parsing error in scene constructor (" + type + " loading)");
                
                const std::string tfile_name_short = std::filesystem::path(tfile_name).filename().generic_string();
                
                printf("Parsing %s...", tfile_name.c_str());
                fflush(stdout);
                bool parsing_successful;
                if (is_texture)
                    texture_wrapper_set.emplace_back(texture(tfile_name, parsing_successful, inverse_gamma), t_name);
                else
                    normal_map_wrapper_set.emplace_back(normal_map(tfile_name, parsing_successful), t_name);

                if (parsing_successful) {
                    printf("\r> %s %s loaded                                \n", tfile_name_short.c_str(), type.c_str());
                    continue;
                }

                printf("%s %s reading failed\n", tfile_name_short.c_str(), type.c_str());
                throw std::runtime_error((type + " reading failed").c_str());
            }

            /* Objects declaration */
            {
                using enum object_type;
                constexpr std::array<std::string, 6> object_type_names_array = {
                    "triangle", "quad", "sphere", "plane", "box", "cylinder"
                };
                const std::span object_type_names(object_type_names_array);
                constexpr std::array object_types = {
                    Triangle, Quad, Sphere, Plane, Box, Cylinder
                };

                const std::optional<unsigned int> index_opt = index_of(arg, object_type_names);
                
                if (index_opt.has_value()) {
                    
                    const unsigned int index = index_opt.value();
                    const object_type type = object_types[index];

                    parse_objects(f, type, arg, containers, bounding_enabled, inverse_gamma);

                    continue;
                }
            }

            /* Obj file parsing */
            if (arg == "load_obj") {
                const std::string ofile_name = f.read_string(MAX_FILENAME_LENGTH);

                double sx = 0, sy = 0, sz = 0, scale = 1;
                std::optional<unsigned int> t_index;

                exit_status status = f.scanf(" (texture:");
                if (status == exit_status::Success) {

                    const std::string t_name = f.read_string(MAX_NAME_LENGTH);
                    exit_status status_shift_scale = f.scanf(" shift:(%lf,%lf,%lf) scale:%lf)\n", sx, sy, sz, scale);
                    throw_if_failure(status_shift_scale, "parsing error in scene constructor (obj file loading)");
                    
                    if (not t_name.starts_with("none")) {
                        t_index = wrapper<texture>::find_element(texture_wrapper_set, t_name);
                        throw_if_null(t_index, "texture not found");
                    }
                }

                const rt::vector shift(sx, sy, sz);
                const bounding* output_bd = nullptr;
                const exit_status status_obj =
                    parse_obj_file(ofile_name, t_index,
                        containers,
                        scale, shift,
                        bounding_enabled, polygons_per_bounding, output_bd, inverse_gamma);

                throw_if_failure(status_obj, ofile_name + " obj file reading failed\n");

                if (bounding_enabled)
                    bounding_set.push_back(output_bd);

                continue;
            }

            /* parsing error */
            throw std::runtime_error(("unexpected keyword " + arg).c_str());
        }

        f.close();

        if (bounding_enabled) {
            // other_content should be tested first, to maximize pruning in the BVH tree-search
            bounding_set.push_back(new bounding(std::move(other_content)));
            std::reverse(bounding_set.begin(), bounding_set.end());
        }

        /***********
        std::cout << "Total boundings: " << bounding::cpt << std::endl;
        std::cout << "Total boxes:     " << bounding::box_type::cpt << std::endl;
        std::cout << "(box type = " << (std::is_same_v<bounding::box_type, box> ? "box" : "aabb") << ")" << std::endl;
        ***********/

        // Creation of the final structures
        auto [ material_set, texture_set, normal_map_set ] = build_sets(material_wrapper_set, texture_wrapper_set, normal_map_wrapper_set);

        background_container&& background = (background_texture_is_set) ?
              background_container(std::move(background_texture), rx, ry, rz)
            : background_container(background_color);

        scene_opt.emplace(
            std::move(object_set),

            std::move(triangle_set),
            std::move(quad_set),
            std::move(sphere_set),
            std::move(plane_set),
            std::move(box_set),
            std::move(cylinder_set),

            std::move(bounding_set),
            std::move(texture_set),
            std::move(normal_map_set),
            std::move(material_set),
            std::move(texture_info_set),
            std::move(background),
            std::move(cam),
            width, height,
            polygons_per_bounding,
            1.0 / inverse_gamma
        );
    }
    catch (const std::exception& e) {
        printf("Error during scene parsing: ");
        printf("%s\n", e.what());
        printf("Scene creation failed\n");
    }
    catch (...) {}

    timer.stop();
    printf("Scene parsing:\n");
    timer.print();

    return scene_opt;
}