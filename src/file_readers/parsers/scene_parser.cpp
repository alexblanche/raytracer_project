#include "file_readers/parsers/scene_parser.hpp"

#include "file_readers/parsers/obj_parser.hpp"

#include "file_readers/file.hpp"
#include "auxiliary/utils.hpp"
// #include "auxiliary/timer.hpp"

#include <string>
#include <sstream>
#include <filesystem>
#include <stdexcept>
#include <tuple>

static constexpr unsigned int MAX_NAME_LENGTH     = 64;
static constexpr unsigned int MAX_FILENAME_LENGTH = 512;

// longest item is load_normal_map, of length 15
static constexpr unsigned int MAX_KEYWORD_LENGTH  = 17;

using enum object_type;

/*** Scene descriptor pre-parsing ***/
[[maybe_unused]] static scene::pre_parsing_info pre_parse(const file& f) {

    scene::pre_parsing_info ppi;
    auto& [ objects, triangles, quads, spheres, planes, boxes, cylinders, materials, mappings ] = ppi;

#if APPLE_CLANG
    const std::span keywords(scene::pre_parsing_info::keywords_array);
#else
    const auto& keywords = scene::pre_parsing_info::keywords_array;
#endif

    std::string arg;
    while (not f.eof()) {
        arg = f.read_string();
        
        std::optional<unsigned int> obj_index_opt = index_of<std::string, 8>(arg, keywords);

        if (obj_index_opt.has_value()) {
            const unsigned int index = obj_index_opt.value();
            switch (index) {
                case 0: triangles++; break;
                case 1: quads++;     break;
                case 2: spheres++;   break;
                case 3: planes++;    break;
                case 4: boxes++;     break;
                case 5: cylinders++; break;
                case 6: materials++; break;
                case 7: mappings++;  break;
                default: break;
            }
            if (index < 6)
                objects++;
        }
        else if (arg == "load_obj") {
            const std::string filename = f.read_string(MAX_FILENAME_LENGTH);
            const auto& [ _, obj_triangles, obj_quads ] = pre_parse_obj(filename);
            triangles += obj_triangles;
            quads     += obj_quads;
            objects   += obj_triangles + obj_quads;
        }

        f.skip_line();
    }
    
    // ppi.print();

    f.rewind();
    return ppi;
}

/*** Scene description parsing ***/

/* Returns width, height */
static std::pair<int, int> parse_resolution(const file& f) {

    /* resolution width:1920 height:1080 */
    std::pair<int, int> res;
    auto& [ width, height ] = res;
    const exit_status status = f.scanf("resolution width:%d height:%d\n", width, height);
    throw_if_failure(status, "parsing error in scene constructor (resolution)");
    return res;
}

static camera parse_camera(const file& f, const int width, const int height) {
        
    /* camera position:(0, 0, 0) direction:(0, 0, 1) rightdir:(1, 0, 0) fov_width:1000 distance:400 [focal_distance:500 aperture:100] (optional) */

    double posx, posy, posz, dx, dy, dz;
    bool depth_of_field_enabled = true;
    
    const exit_status st_posdir = f.scanf("camera position:(%lf,%lf,%lf) direction:(%lf,%lf,%lf)",
        posx, posy, posz, dx, dy, dz);
    throw_if_failure(st_posdir, "parsing error in scene constructor (camera)");

    double rdx, rdy, rdz;
    const exit_status st_r = f.scanf(" rightdir:(%lf,%lf,%lf)", rdx, rdy, rdz);
    if (st_r == exit_status::Failure) {
        const exit_status st_r_auto = f.scanf("auto");
        throw_if_failure(st_r_auto, "parsing error in scene constructor (camera right direction)");

        /* Automatic determination of the right direction */
        rdy = 0.0;
        if (dx == 0.0 && dz == 0.0) {
            rdx = 1.0;
            rdz = 0.0;
        }
        else {
            rdx = -dz;
            rdz = dx;
        }
    }

    double fovw, dist, focl, apr;
    const int ret = f.scanf_count(" fov_width:%lf distance:%lf focal_distance:%lf aperture:%lf\n",
        fovw, dist, focl, apr);
    
    if (ret < 2)
        throw std::runtime_error("parsing error in scene constructor (camera fov)");

    if (ret == 2) // Focal length and aperture omitted
        depth_of_field_enabled = false;

    if (f.peek_next() == '#')
        f.skip_line();

    const rt::vector cam_pos(posx, posy, posz);
    const rt::vector cam_dir(dx, dy, dz);
    const rt::vector cam_right_dir(rdx, rdy, rdz);

    const real fovh = fovw * static_cast<real>(height) / static_cast<real>(width);

    return depth_of_field_enabled ?
          camera(cam_pos, cam_dir, cam_right_dir, fovw, fovh, dist, width, height, focl, apr)
        : camera(cam_pos, cam_dir, cam_right_dir, fovw, fovh, dist, width, height);
}

struct bg_parsing_result {
    background_container bg;
    std::optional<real> inverse_gamma;
};

static bg_parsing_result parse_background(const file& f) {
    
/*
    - At least one of background_color, background_texture must be specified
    - filename.bmp or filename.hdr should designate a panoramic image
    - All angles must be between 0 and 2*pi
    - The gamma correction will be applied to the whole picture, so all the non-background colors
        (including the textures) are first corrected with the inverse gamma correction.
        Gamma is optional.
    - fov_height is generated automatically (for width/height aspect ratio)
*/
    bool background_texture_is_set = false;
    rt::color background_color;
    texture background_texture;
    std::optional<real> inverse_gamma;

    // Setting up the background_color or texture
    double r, g, b;
    char bg_tfile_name[513];
    double rx = 0.0, ry = 0.0, rz = 0.0, inverse_gamma_val = 1.0;
    
    /*
        background_color 190 235 255
        background_texture filename.hdr rotate_x:3.14 rotate_y:5.835 rotate_z:0 gamma:2.2
    */
    const exit_status status_background = f.scanf_rewind_if_failure("background_color %lf %lf %lf\n", r, g, b);
    if (status_background == exit_status::Success) {
        background_color = rt::color(r, g, b);
    }

    const int pos = f.position();
    const exit_status status_bg_texture = f.scanf("background_texture %512s", bg_tfile_name);

    if (status_bg_texture == exit_status::Failure) {
        f.rewind(pos);
    }
    else {

        const exit_status status_rotate =
               f.scanf_rewind_if_failure("rotate_x:%lf", rx)
            || f.scanf_rewind_if_failure("rotate_y:%lf", ry)
            || f.scanf_rewind_if_failure("rotate_z:%lf", rz);
        
        // std::cout << "next = " << f.peek_next() << std::endl;

        const exit_status status_gamma = f.scanf(" gamma:%lf", inverse_gamma_val);

        if (status_gamma == exit_status::Success && inverse_gamma_val != 1.0)
            inverse_gamma = inverse_gamma_val;

        if (status_rotate == exit_status::Success) {
            if (std::abs(rx) > 2.0_r * PI || std::abs(ry) > 2.0_r * PI || std::abs(rz) > 2.0_r * PI)
                throw std::runtime_error("incorrect background texture angles");
            
            if (rx < 0) rx += 2.0_r * PI;
            if (ry < 0) ry += 2.0_r * PI;
            if (rz < 0) rz += 2.0_r * PI;
        }
        else {
            rx = 0;
            ry = 0;
            rz = 0;
        }

        // std::cout << "rotate = " << ((status_rotate == exit_status::Success) ? "Success" : "Failure") << std::endl;
        // // std::cout << "gamma = " << ((status_gamma == exit_status::Success) ? "Success" : "Failure") << std::endl;
        // std::cout << "ret_gamma = " << ret_gamma << std::endl;

        const std::string bg_tfile_name_short = std::filesystem::path(bg_tfile_name).filename().generic_string();

        printf("Parsing %s... ", bg_tfile_name_short.c_str());
        fflush(stdout);

        try {
            background_texture = texture(bg_tfile_name);
        }
        catch (const std::exception& e) {
            printf("%s\n", e.what());
            throw std::runtime_error("parsing error in scene constructor (background texture parsing)");
        }           
        
        printf("\r> %s texture loaded\n", bg_tfile_name_short.c_str());
        background_texture_is_set = true;
    }

    if (f.peek_next() == '#' || exit_status::Success == f.scanf_rewind_if_failure("background_color"))
        f.skip_line();

    return {
        .bg = (background_texture_is_set) ?
              background_container(std::move(background_texture), rx, ry, rz)
            : background_container(background_color),
        .inverse_gamma = inverse_gamma
    };
}

static unsigned int parse_bvh(const file& f) {
    
    /*
        polygons_per_bounding 10 //specifying 0 will deactivate the bounding generation
        or
        bvh: polygons_per_bounding 10
        or
        bvh: disabled
    */
    f.skip_whitespace();
    f.scanf_rewind_if_failure("bvh: ");
    unsigned int polygons_per_bounding = 0;
    const exit_status status = f.scanf("polygons_per_bounding %u\n", polygons_per_bounding);
    if (status == exit_status::Failure) {
        throw_if_failure(f.scanf("disabled"),
            "parsing error in scene constructor (BVH parameters)");
    }

    return polygons_per_bounding;
}

/* Auxiliary function: returns a material from a description file */
static std::optional<material> parse_material(const file& f, const std::optional<real> gamma) {
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
    if (gamma.has_value()) {
        mat_color.apply_gamma(gamma.value());
        // em_color.apply_gamma(gamma.value());
    }

    return material(mat_color, mp.smooth, mp.em_int, mp.refl, mp.refl_color, mp.transp, mp.scattering, mp.refr_i);
}


/* Auxiliary function: parses the name of a material and returns its index in material_set,
   or parses a new material, stores it in material_set and returns its index */
static std::optional<unsigned int> get_material(const file& f, std::vector<wrapper<material>>& material_wrapper_set, const std::optional<real> gamma) {

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

static void parse_mapping(const file& f, const std::optional<real> inverse_gamma,
    containers& containers) {

    auto& [
        _, _, _, _,
        composition_wrapper_set,
        texture_set,
        normal_map_set,
        _
    ]
    = containers;

    const std::string mapping_name = f.read_string(MAX_NAME_LENGTH);
    if (mapping_name.length() == 0)
        throw std::runtime_error("parsing error: mapping name");

    if (wrapper<composition>::find_element(composition_wrapper_set, mapping_name, silent_option::Silent).has_value())
        throw std::runtime_error("error: mapping " + mapping_name + " already defined\n");

    composition comp;

    while (not f.eof()) {

        const std::size_t pos = f.position();
        const std::string arg = f.read_string(MAX_KEYWORD_LENGTH);
        if (arg != "texture" && arg != "normal_map") {
            f.rewind(pos);
            break;
        }

        enum class type {
            Texture, Normal_map // ...
        };
        using enum type;
        const auto to_string = [] (type type_) -> std::string {
            switch (type_) {
                case Texture:    return "texture";
                case Normal_map: return "normal map";
                // ...
                default: throw;
            }
        };

        const type type_ = (arg == "texture") ? Texture :
            // ...
            Normal_map;
        static_assert(TODO_ROUGHNESS_MAP);
        static_assert(TODO_DISPLACEMENT_MAP);

        const std::string type_str = to_string(type_);

        if (   (type_ == Texture    && comp.has_texture)
            || (type_ == Normal_map && comp.has_normal_map))
            // ...
            throw std::runtime_error("parsing error: " + type_str + " already defined in mapping " + mapping_name + "\n");

        const std::string tfile_name = f.read_string(MAX_FILENAME_LENGTH);
        if (tfile_name.length() == 0)
            throw std::runtime_error("parsing error in mapping definition " + type_str + ")");
        const std::string tfile_name_short = std::filesystem::path(tfile_name).filename().generic_string();
        
        printf("Parsing %s...", tfile_name.c_str());
        fflush(stdout);

        try {
            switch (type_) {
                case Texture: {
                    texture_set.emplace_back(tfile_name, inverse_gamma);
                    comp.has_texture = true;
                    break;
                }
                case Normal_map: {
                    normal_map_set.emplace_back(tfile_name);
                    comp.has_normal_map = true;
                    break;
                }
                // ...
                default:
                    static_assert(TODO_ROUGHNESS_MAP);
                    static_assert(TODO_DISPLACEMENT_MAP);
                    throw;
            }

            printf("\r> %s %s loaded                                                     \n",
                tfile_name_short.c_str(), type_str.c_str());
        }
        catch (const std::exception& e) {
            printf("%s %s reading failed\n", tfile_name_short.c_str(), type_str.c_str());
            throw std::runtime_error(type_str + " reading failed");
        }
    }

    // Complete the undefined mappings
    if (not comp.has_texture)    texture_set   .emplace_back();
    if (not comp.has_normal_map) normal_map_set.emplace_back();
    // ...
    static_assert(TODO_ROUGHNESS_MAP);
    static_assert(TODO_DISPLACEMENT_MAP);

    composition_wrapper_set.emplace_back(comp, mapping_name);
}

/* Auxiliary function: returns the common index of the texture or normal map, roughness map or displacement map
    and the composition (see mapping_info)
*/
static std::optional<mapping::index_type> parse_mapping_index(
    const file& f, const std::vector<wrapper<composition>>& composition_wrapper_set) {

    const exit_status status_t = f.scanf_rewind_if_failure("mapping:(");

    if (status_t == exit_status::Failure)
        return std::nullopt;

    std::string t_name = f.read_string(MAX_NAME_LENGTH);
    if (t_name.ends_with(')'))
        t_name = t_name.substr(0, t_name.length() - 1);
    return wrapper<composition>::find_element(composition_wrapper_set, t_name);
}
    
static triangle::orientation parse_triangle_orientation(const file& f,
    const unsigned int index, const rt::vector& tr_v1, const rt::vector& tr_v2) {

    double u0, v0, u1, v1, u2, v2;
    const exit_status status = f.scanf(" (%lf,%lf) (%lf,%lf) (%lf,%lf))\n",
        u0, v0, u1, v1, u2, v2);
    
    if (status == exit_status::Failure)
        throw std::runtime_error("parsing error in parse_texture_info (triangle UV-coordinates)\n");

    return triangle::orientation(index, { uvcoord
        { u0, 1.0_r - v0 },
        { u1, 1.0_r - v1 },
        { u2, 1.0_r - v2 } },
        tr_v1, tr_v2
    );
}

static quad::orientation parse_quad_orientation(const file& f,
    const unsigned int index, const rt::vector& q_v1, const rt::vector& q_v2) {

    double u0, v0, u1, v1, u2, v2, u3, v3;
    const exit_status status = f.scanf(" (%lf,%lf) (%lf,%lf) (%lf,%lf) (%lf,%lf))\n",
        u0, v0, u1, v1, u2, v2, u3, v3);
    
    if (status == exit_status::Failure) {
        // Default values: (0,1) (0,0) (1,0) (1,1)
        u0 = 0; v0 = 1;
        u1 = 0; v1 = 0;
        u2 = 1; v2 = 0;
        u3 = 1; v3 = 1;
    }

    return quad::orientation(index, { uvcoord
        { u0, 1.0_r - v0 },
        { u1, 1.0_r - v1 },
        { u2, 1.0_r - v2 },
        { u3, 1.0_r - v3 } },
        q_v1, q_v2
    );
}

static sphere::orientation parse_sphere_orientation(const file& f, const unsigned int index) {

    double fx, fy, fz, rx, ry, rz;
    const exit_status status = f.scanf(" forward:(%lf,%lf,%lf) right:(%lf,%lf,%lf))\n",
        fx, fy, fz, rx, ry, rz);

    if (status == exit_status::Failure) {
        // Default values: forward = (0,0,-1), right = (1,0,0)
        fx = 0; fy = 0; fz = -1;
        rx = 1; ry = 0; rz = 0;
    }

    return sphere::orientation(index, rt::vector(fx, fy, fz), rt::vector(rx, ry, rz));
}

static plane::orientation parse_plane_orientation(const file& f,
    const unsigned int index, const rt::vector& normal) {

    double rx, ry, rz;
    const exit_status status_right = f.scanf(" right:(%lf,%lf,%lf)",
        rx, ry, rz);
    
    if (status_right == exit_status::Failure) {
        // Default values: right = (1, 0, 0), scale = 1
        rx = 1; ry = 0; rz = 0;
    }

    double scale = 1.0_r; // Default value
    f.scanf(" scale:%lf)\n", scale);

    return plane::orientation(index, normal, rt::vector(rx, ry, rz), scale);
}

/*
static box::orientation parse_box_orientation(const file& f, const unsigned int index) {

    static_assert(TODO_BOX_TEXTURING);
    throw std::runtime_error("box texturing not handled yet");
}

static cylinder::orientation parse_cylinder_orientation(const file& f, const unsigned int index) {

    static_assert(TODO_CYLINDER_TEXTURING);
    throw std::runtime_error("cylinder texturing not handled yet");
}
*/

static void parse_objects(const file& f, const object_type type, const std::string& arg,
        containers& containers, const bool bounding_enabled, std::optional<real> inverse_gamma) {
    
    union object_constructor_parameters {
        struct { rt::vector p[3]; }                                   triangle;
        struct { rt::vector p[4]; }                                   quad;
        struct { rt::vector center; real radius; }                    sphere;
        struct { rt::vector position, normal; }                       plane;
        struct { rt::vector center, x_axis, y_axis; real l[3]; }      box;
        struct { rt::vector origin, direction; real radius, length; } cylinder;

        object_constructor_parameters() {}
    };
    object_constructor_parameters parameters;
    exit_status status;

    using enum object_type;
    switch (type) {

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

        default: throw;
    }

    throw_if_failure(status, "parsing error in scene constructor (" + arg + " declaration)");

    auto& [
        object_set,
        other_content,
        object_containers,
        material_wrapper_set,
        composition_wrapper_set,
        texture_set,
        normal_map_set,
        orientation_containers
    ]
    = containers;

    auto& [
        triangle_set,
        quad_set,
        sphere_set,
        plane_set,
        box_set,
        cylinder_set
    ]
    = object_containers;

    const std::optional<unsigned int> m_index_opt = get_material(f, material_wrapper_set, inverse_gamma);
    throw_if_nullopt(m_index_opt, "material definition error");

    const unsigned int m_index = m_index_opt.value();

    const object* obj = nullptr;

    static_assert(TODO_BOX_TEXTURING);
    static_assert(TODO_CYLINDER_TEXTURING);

    if (type == Box || type == Cylinder) {

        switch (type) {

            case Box: {
                const auto& [ center, x_axis, y_axis, l ] = parameters.box;
                const auto& [ lx, ly, lz ] = l;
                obj = &box_set.emplace_back(center, x_axis, y_axis, lx, ly, lz, m_index);
                break;
            }

            case Cylinder: {
                const auto& [ origin, direction, radius, length ] = parameters.cylinder;
                obj = &cylinder_set.emplace_back(origin, direction, radius, length, m_index);
                break;
            }

            default: throw;
        }
    }
    else {
        //std::optional<texture_info> info = parse_texture_info(f, texture_wrapper_set, normal_map_wrapper_set, type);
        const std::optional<mapping::index_type> index_opt = parse_mapping_index(f, composition_wrapper_set);
        
        if (index_opt.has_value()) {

            const mapping::index_type index = index_opt.value();

            auto& [
                triangle_orientation_set,
                quad_orientation_set,
                sphere_orientation_set,
                plane_orientation_set,
                _, _
            ] = containers.orientation_containers;

            switch (type) {

                case Triangle: {
                    const auto& [ p ] = parameters.triangle;
                    triangle::orientation orientation = parse_triangle_orientation(f, index, p[1] - p[0], p[2] - p[0]);
                    const unsigned int orientation_index = triangle_orientation_set.size();
                    obj = &triangle_set.emplace_back(p[0], p[1], p[2], m_index, orientation_index);
                    triangle_orientation_set.emplace_back(std::move(orientation));
                    break;
                }

                case Quad: {
                    const auto& [ p ] = parameters.quad;
                    quad::orientation orientation = parse_quad_orientation(f, index, p[1] - p[0], p[2] - p[0]);
                    const unsigned int orientation_index = quad_orientation_set.size();
                    obj = &quad_set.emplace_back(p[0], p[1], p[2], p[3], m_index, orientation_index);
                    quad_orientation_set.emplace_back(std::move(orientation));
                    break;
                }

                case Sphere: {
                    const auto& [ center, radius ] = parameters.sphere;
                    sphere::orientation orientation = parse_sphere_orientation(f, index);
                    const unsigned int orientation_index = sphere_orientation_set.size();
                    obj = &sphere_set.emplace_back(center, radius, m_index, orientation_index);
                    sphere_orientation_set.emplace_back(std::move(orientation));
                    break;
                }

                case Plane: {
                    const auto& [ position, normal ] = parameters.plane;
                    plane::orientation orientation = parse_plane_orientation(f, index, normal.unit());
                    const unsigned int orientation_index = plane_orientation_set.size();
                    obj = &plane_set.emplace_back(normal, position, m_index, orientation_index);
                    plane_orientation_set.emplace_back(std::move(orientation));
                    break;
                }

                default: throw;
            }
        }
        else {

            switch (type) {

                case Triangle: {
                    const auto& [ p ] = parameters.triangle;
                    obj = &triangle_set.emplace_back(p[0], p[1], p[2], m_index);
                    break;
                }

                case Quad: {
                    const auto& [ p ] = parameters.quad;
                    obj = &quad_set.emplace_back(p[0], p[1], p[2], p[3], m_index);
                    break;
                }

                case Sphere: {
                    const auto& [ center, radius ] = parameters.sphere;
                    obj = &sphere_set.emplace_back(center, radius, m_index);
                    break;
                }
                
                case Plane: {
                    const auto& [ position, normal ] = parameters.plane;
                    obj = &plane_set.emplace_back(normal, position, m_index);
                    break;
                }

                default: throw;
            }
        }
    }

    object_set.push_back(obj);

    if (bounding_enabled)
        other_content.push_back(obj);
}

struct parse_load_obj_result {
    std::string ofile_name;
    std::optional<mapping::index_type> m_index;
    model_positioning positioning;
};

static parse_load_obj_result parse_load_obj(
    const file& f, const std::vector<wrapper<composition>>& composition_wrapper_set) {
    
    parse_load_obj_result res;
    auto& [ ofile_name, m_index, positioning ] = res;
    
    ofile_name = f.read_string(MAX_FILENAME_LENGTH);

    if (f.scanf(" (mapping:") == exit_status::Failure)
        return res;
    
    std::string m_name = f.read_string(MAX_NAME_LENGTH);
    
    if (not m_name.starts_with("none")) {
        if (m_name.ends_with(')'))
            m_name.resize(m_name.size() - 1);
        m_index = wrapper<composition>::find_element(composition_wrapper_set, m_name);
        throw_if_nullopt(m_index, "mapping not found");
    }

    double sx, sy, sz, scale;
    const int ret = f.scanf_count("shift:(%lf,%lf,%lf) scale:%lf)\n", sx, sy, sz, scale);

    if (ret >= 3) {
        const rt::vector shift = rt::vector(sx, sy, sz);
        positioning = (ret == 4) ?
              model_positioning(shift, scale)
            : model_positioning(shift);
    }

    return res;
}


/** Scene description parser **/

std::optional<scene> parse_scene_descriptor(const std::string& file_name) {

    timer_ms timer;
    timer.start();

    std::optional<scene> scene_opt;

    try {

        file f(file_name, "rb");

        const scene::pre_parsing_info pre_parsing_info = pre_parse(f);
        auto [ width, height ] = parse_resolution(f);
        camera cam = parse_camera(f, width, height);
        auto [ background, inverse_gamma ] = parse_background(f);
        unsigned int polygons_per_bounding = parse_bvh(f);

        std::vector<const object*> object_set;
        object_set.reserve(pre_parsing_info.max_objects());

        scene::containers::object      object_containers     (pre_parsing_info);
        scene::containers::orientation orientation_containers(pre_parsing_info);

        /* Material storage */
        std::vector<wrapper<material>> material_wrapper_set;
        material_wrapper_set.emplace_back(DIFFUSE, "diffuse");
        material_wrapper_set.emplace_back(MIRROR,  "mirror" );
        material_wrapper_set.emplace_back(GLASS,   "glass"  );
        material_wrapper_set.emplace_back(WATER,   "water"  );

        std::vector<wrapper<mapping::composition>> composition_wrapper_set;
        std::vector<texture>    texture_set;
        std::vector<normal_map> normal_map_set;
        static_assert(TODO_ROUGHNESS_MAP);
        static_assert(TODO_DISPLACEMENT_MAP);

        std::vector<const bounding*> bounding_set;

        /* Bounding handling */
        /* When bvh is disabled, the objects that are not defined in an obj file are placed in
        the vector other_content. At the end, these objects are placed in a bounding alongside the ones generated during obj files parsing */
        std::vector<const object*> other_content;
        other_content.reserve(pre_parsing_info.total_non_polygon_objects());
        const bool bounding_enabled = polygons_per_bounding != 0;

        containers containers = {
            object_set,
            other_content,
            object_containers,
            material_wrapper_set,
            composition_wrapper_set,
            texture_set,
            normal_map_set,
            orientation_containers
        };

        /* Parsing loop */

        while (not f.eof()) {

            /* Commented line */
            f.skip_whitespace();
            if (f.peek_next() == '#') {
                f.skip_line();
                continue;
            }

            const std::string arg = f.read_string(MAX_KEYWORD_LENGTH);

            if (f.eof())
                break;
            
            /* Material declaration */
            if (arg == "material") {
                const std::string m_name = f.read_string(MAX_NAME_LENGTH);

                std::optional<material> m = parse_material(f, inverse_gamma);
                throw_if_nullopt(m, "material parsing error");
                
                material_wrapper_set.emplace_back(std::move(m.value()), m_name);
                continue;
            }
            
            /* BMP file loading */
            if (arg == "load_mapping") {

                parse_mapping(f, inverse_gamma, containers);
                continue;
            }

            /* Objects declaration */
            {
                using enum object_type;
                
#if APPLE_CLANG
                constexpr std::array<std::string, 6> object_type_names_array = {
                    "triangle", "quad", "sphere", "plane", "box", "cylinder"
                };
                const std::span object_type_names(object_type_names_array);
#else
                static const std::array<std::string, 6> object_type_names_array = {
                    "triangle", "quad", "sphere", "plane", "box", "cylinder"
                };
                const auto& object_type_names = object_type_names_array;
#endif

                constexpr std::array object_types = {
                    Triangle, Quad, Sphere, Plane, Box, Cylinder
                };

                const std::optional<unsigned int> index_opt = index_of(arg, object_type_names);
                
                if (index_opt.has_value()) {
                    
                    const object_type type = object_types[index_opt.value()];
                    parse_objects(f, type, arg, containers, bounding_enabled, inverse_gamma);
                    continue;
                }
            }

            /* Obj file parsing */
            if (arg == "load_obj") {
                
                const auto& [ ofile_name, m_index, positioning ] = parse_load_obj(f, composition_wrapper_set);

                const bounding* output_bd = nullptr;
                const exit_status status_obj =
                    parse_obj_file(ofile_name, m_index,
                        containers, positioning,
                        bounding_enabled, polygons_per_bounding, output_bd,
                        inverse_gamma);

                throw_if_failure(status_obj, ofile_name + " obj file reading failed\n");

                if (bounding_enabled)
                    bounding_set.push_back(output_bd);

                continue;
            }

            /* parsing error */
            throw std::runtime_error("unexpected keyword " + arg);
        }

        f.close();

        if (bounding_enabled) {
            // other_content should be tested first, to maximize pruning in the BVH tree-search
            bounding_set.push_back(new bounding(std::move(other_content)));
            std::reverse(bounding_set.begin(), bounding_set.end());
        }

        // wrapper<material>::print_content(material_wrapper_set);
        
        // Creation of the final structures
        auto [ material_set, comp_set ] = build_sets(material_wrapper_set, composition_wrapper_set);

        scene::containers::mapping mapping_containers(
            std::move(material_set),
            std::move(comp_set),
            std::move(texture_set),
            std::move(normal_map_set),
            std::move(background)
        );

        const std::optional<real> gamma = (inverse_gamma.has_value()) ?
              std::optional(1.0_r / inverse_gamma.value())
            : std::nullopt;

        scene_opt.emplace(
            std::move(object_set),
            std::move(bounding_set),
            std::move(object_containers),
            std::move(mapping_containers),
            std::move(orientation_containers),
            std::move(cam),
            width, height,
            polygons_per_bounding,
            gamma
        );
    }
    catch (const std::exception& e) {
        printf("Error during scene parsing: ");
        printf("%s\n", e.what());
        printf("Scene creation failed\n");
    }
    catch (...) {}

    timer.stop();
    printf("Scene parsing: ");
    timer.print();

    return scene_opt;
}