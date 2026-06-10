#include "file_readers/scene_parser.hpp"

#include "file_readers/obj_parser.hpp"

#include "scene/objects/bounding.hpp"
#include "scene/objects/sphere.hpp"
#include "scene/objects/plane.hpp"
#include "scene/objects/box.hpp"
#include "scene/objects/triangle.hpp"
#include "scene/objects/quad.hpp"
#include "scene/objects/cylinder.hpp"

#include "file_readers/parsing_wrappers.hpp"
#include "scene/material/normal_map.hpp"

#include "file_readers/file.hpp"

#include <string>
#include <sstream>
#include <filesystem>
#include <stdexcept>


/*** Scene description parsing ***/

/* Auxiliary function: returns a material from a description file */
std::optional<material> parse_material(const file& f, const real gamma) {
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

    /*
    const int ret = fscanf(file, "color:(%lf,%lf,%lf) smoothness:%lf emission:%lf reflectivity:%lf reflects_color:%5s transparency:%lf scattering:%lf refraction_index:%lf)", 
        &r, &g, &b, &er, &eg, &eb, &refl, &em_int, &spec_p, refl_c, &transp, &scattering, &refr_i);

    if (ret != 13) {
        printf("Parsing error in parse_material\n");
        return scene_opt;
    }
    */
    constexpr int BUFFER_MAX_SIZE = 256;
    char buffer[BUFFER_MAX_SIZE];
    //const int ret = fscanf(file, "%255[^\n]", buffer);
    // Manual extraction (to avoid going over texture declaration)
    int depth = 0;
    int i;
    for (i = 0; i < BUFFER_MAX_SIZE; i++) {
        const char c = f.getc();
        if (c == '(')
            depth++;
        else if (c == ')') {
            if (depth == 0)
                break;
            depth--;
        }
        buffer[i] = c;
    }
    
    if (i == BUFFER_MAX_SIZE) {
        printf("Parsing error in parse_material: material definition is too long\n");
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

    std::istringstream stream(buffer); 
    std::string word;
    while (stream >> word) {
        if (is_set.nb_param >= 9) {
            printf("Parsing error in parse_material: too many parameters set\n");
            return std::nullopt;
        }

        if (word.starts_with("diffuse")) {
            if (is_set.nb_param) {
                printf("Parsing error in parse_material: no parameter should be set in addition to diffuse\n");
                return std::nullopt;
            }
            // Default is diffuse white
            break;
        }
        else if (word.starts_with("mirror")) {
            if (is_set.nb_param) {
                printf("Parsing error in parse_material: no parameter should be set in addition to mirror\n");
                return std::nullopt;
            }
            mp = copy_material(MIRROR);
            break;
        }
        else if (word.starts_with("glass")) {
            if (is_set.nb_param) {
                printf("Parsing error in parse_material: no parameter should be set in addition to glass\n");
                return std::nullopt;
            }
            mp = copy_material(GLASS);
            break;
        }
        else if (word.starts_with("water")) {
            if (is_set.nb_param) {
                printf("Parsing error in parse_material: no parameter should be set in addition to water\n");
                return std::nullopt;
            }
            mp = copy_material(WATER);
            break;
        }

        if (word.starts_with("color:")) {
            if (is_set.color) {
                printf("Parsing error in parse_material: duplicate color definition\n");
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
                printf("Parsing error in parse_material: color\n");
                return std::nullopt;
            }
            is_set.color = true;
            is_set.nb_param++;
        }

        /*
        else if (word.starts_with("emitted_color:")) {
            if (is_set.em_color) {
                printf("Parsing error in parse_material: duplicate emitted color definition\n");
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
                printf("Parsing error in parse_material: emitted color\n");
                return std::nullopt;
            }
            is_set.em_color = true;
            is_set.nb_param++;
        }
        */

        else if (word.starts_with("smoothness:")) {
            if (is_set.smooth) {
                printf("Parsing error in parse_material: duplicate smoothness definition\n");
                return std::nullopt;
            }
            const int ret = sscanf(word.data(), "smoothness:%lf", &mp.smooth);
            if (ret != 1) {
                printf("Parsing error in parse_material: smoothness\n");
                return std::nullopt;
            }
            is_set.smooth = true;
            is_set.nb_param++;
        }

        else if (word.starts_with("emission:")) {
            if (is_set.em_int) {
                printf("Parsing error in parse_material: duplicate emission definition\n");
                return std::nullopt;
            }
            const int ret = sscanf(word.data(), "emission:%lf", &mp.em_int);
            if (ret != 1) {
                printf("Parsing error in parse_material: emission\n");
                return std::nullopt;
            }
            is_set.em_int = true;
            is_set.nb_param++;
        }

        else if (word.starts_with("reflectivity:")) {
            if (is_set.refl) {
                printf("Parsing error in parse_material: duplicate reflectivity definition\n");
                return std::nullopt;
            }
            const int ret = sscanf(word.data(), "reflectivity:%lf", &mp.refl);
            if (ret != 1) {
                printf("Parsing error in parse_material: reflectivity\n");
                return std::nullopt;
            }
            is_set.refl = true;
            is_set.nb_param++;
        }

        else if (word.starts_with("transparency:")) {
            if (is_set.transp) {
                printf("Parsing error in parse_material: duplicate transparency definition\n");
                return std::nullopt;
            }
            const int ret = sscanf(word.data(), "transparency:%lf", &mp.transp);
            if (ret != 1) {
                printf("Parsing error in parse_material: transparency\n");
                return std::nullopt;
            }
            is_set.transp = true;
            is_set.nb_param++;
        }

        else if (word.starts_with("scattering:")) {
            if (is_set.scattering) {
                printf("Parsing error in parse_material: duplicate scattering definition\n");
                return std::nullopt;
            }
            const int ret = sscanf(word.data(), "scattering:%lf", &mp.scattering);
            if (ret != 1) {
                printf("Parsing error in parse_material: scattering\n");
                return std::nullopt;
            }
            is_set.scattering = true;
            is_set.nb_param++;
        }

        else if (word.starts_with("refraction_index:")) {
            if (is_set.refr_i) {
                printf("Parsing error in parse_material: duplicate refraction_index definition\n");
                return std::nullopt;
            }
            const int ret = sscanf(word.data(), "refraction_index:%lf", &mp.refr_i);
            if (ret != 1) {
                printf("Parsing error in parse_material: refraction_index\n");
                return std::nullopt;
            }
            is_set.refr_i = true;
            is_set.nb_param++;
        }

        else if (word.starts_with("reflects_color:")) {
            if (is_set.refl_col) {
                printf("Parsing error in parse_material: duplicate reflects_color definition\n");
                return std::nullopt;
            }
            if (word.starts_with("reflects_color:true")) {
                mp.refl_color = true;
            }
            else if (word.starts_with("reflects_color:false")) {
                mp.refl_color = false;
            }
            else {
                printf("Parsing error in parse_material: reflects_color\n");
                return std::nullopt;
            }
            is_set.refl_col = true;
            is_set.nb_param++;
        }

        else if (word.starts_with("texture:") || word.starts_with(")")) {
            break;
        }
        
        else {
            printf("Parsing error in parse_material: %s\n", word.data());
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
std::optional<unsigned int> get_material(const file& f, std::vector<wrapper<material>>& material_wrapper_set, const real gamma) {

    const char firstchar = f.getc();
    
    if (firstchar == '(') {
        // material declaration
        std::optional<material> m = parse_material(f, gamma);

        if (m.has_value()) {
            material_wrapper_set.emplace_back(std::move(m.value()));
            return material_wrapper_set.back().index;
        }
        else {
            return std::nullopt;
        }
    }
    else {
        // Moving back the pointer back by one position
        f.ungetc(firstchar);

        // material variable name
        const std::string vname = f.read_string(64);
        return wrapper<material>::find_element(material_wrapper_set, vname);
    }
}

/* Auxiliary function: parses the texture information after a material, and if there is one,
   returns the associated texture_info
   is_triangle is true if the object is a triangle (in this case, 3 uv points are parsed),
   and false if it is a quad (4 uv points are parsed) */
std::optional<texture_info> parse_texture_info(const file& f,
    const std::vector<wrapper<texture>>& texture_wrapper_set,
    const std::vector<wrapper<normal_map>>& normal_map_wrapper_set,
    const object_type object_type) {

    const size_t position = f.position();
    f.skip_char(' ');
    const std::string keyword_t = f.read_string(7);

    if (keyword_t != "texture") {
        // No texture info, setting the position back before the keyword
        f.rewind(position);
        return std::nullopt;
    }
    
    std::optional<unsigned int> nindex;
    double u0, v0, u1, v1, u2, v2, u3, v3;
    double x0, y0, z0, x1, y1, z1;

    char t_name[65];
    const exit_status status_t = f.scanf(":(%64s", t_name);
    if (status_t == exit_status::Failure) {
        printf("Parsing error in parse_texture_info (texture name)\n");
        return std::nullopt;
    }

    const std::optional<unsigned int> vindex = wrapper<texture>::find_element(texture_wrapper_set, t_name);

    const std::size_t pos2 = f.position();
    const std::string keyword_n = f.read_string(7);
    if (keyword_n != "normal:") {
        // No normal map info, setting the position back before the keyword
        f.rewind(pos2);
    }
    else {
        char n_name[65];
        const exit_status status = f.scanf("%64s", n_name);
        if (status == exit_status::Failure) {
            printf("Parsing error in parse_texture_info (normal map name)\n");
            return std::nullopt;
        }
        nindex = wrapper<normal_map>::find_element(normal_map_wrapper_set, n_name); 
    }

    /*
    // Roughness map

    const std::size_t pos3 = f.position();
    const std::string keyword_r = f.read_string(6);
    if (keyword_r != "rough:") {
        // No roughness info, setting the position back before the keyword
        f.rewind(pos3);
    }
    else {
        char r_name[65];
        const exit_status status_r = f.scanf("%64s", r_name);
        if (status_r == exit_status::Failure) {
            printf("Parsing error in parse_texture_info (roughness map name)\n");
            return std::nullopt;
        }

        // Finding the index associated with the normal map name
        for (wrapper<roughness_map> const& rm_wrap : roughness_map_wrapper_set) {
            if (rm_wrap.name.has_value() && rm_wrap.name.value().compare(r_name) == 0) {
                rindex = rm_wrap.index;
                break;
            }
        }
        if (not rindex.has_value()) {
            printf("Error, roughness map %s not found\n", r_name);
            return std::nullopt;
        }

        printf("Roughness map : %s, index %lld\n", r_name, rindex.value());
    }
    */
    using enum object_type;
    switch (object_type) {
        case Triangle: {

            const exit_status status = f.scanf(" (%lf,%lf) (%lf,%lf) (%lf,%lf))\n",
                u0, v0, u1, v1, u2, v2);
            if (status == exit_status::Failure) {
                printf("Parsing error in parse_texture_info (triangle UV-coordinates)\n");
                return std::nullopt;
            }

            return texture_info(vindex, nindex, { u0, v0, u1, v1, u2, v2 });
        }
                    
        case Quad: {

            const exit_status status = f.scanf(" (%lf,%lf) (%lf,%lf) (%lf,%lf) (%lf,%lf))\n",
                u0, v0, u1, v1, u2, v2, u3, v3);
            if (status == exit_status::Failure) {
                printf("Parsing error in parse_texture_info (quad UV-coordinates)\n");
                return std::nullopt;
            }

            return texture_info(vindex, nindex, { u0, v0, u1, v1, u2, v2, u3, v3 });
        }

        case Sphere: {

            const exit_status status = f.scanf(" forward:(%lf,%lf,%lf) right:(%lf,%lf,%lf))\n",
                x0, y0, z0, x1, y1, z1);
            if (status == exit_status::Failure) {
                printf("Parsing error in parse_texture_info (sphere forward and right directions)\n");
                return std::nullopt;
            }

            // texture_info is used to pass the coordinates for forward_dir and right_dir
            return texture_info(vindex, nindex, { x0, y0, z0, x1, y1, z1 });
        }

        case Plane: {

            const exit_status status = f.scanf(" right:(%lf,%lf,%lf) scale:%lf)\n",
                x0, y0, z0, u0);
            if (status == exit_status::Failure) {
                printf("Parsing error in parse_texture_info (plane right direction and scale)\n");
                return std::nullopt;
            }

            // texture_info is used to pass the coordinates for forward_dir and right_dir
            return texture_info(vindex, nindex, { x0, y0, z0, u0 });
        }

        case Box:
            throw std::runtime_error("Box texturing not handled yet");

        case Cylinder:
            throw std::runtime_error("Cylinder texturing not handled yet");

        default:
            throw std::runtime_error("Incorrect object type");
    }
}


/** Scene description parser **/

std::optional<scene> parse_scene_descriptor(const std::string& file_name) {

    file f(file_name);

    std::optional<scene> scene_opt;

    try {

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

        const exit_status status1 = f.scanf("resolution width:%d height:%d\n", width, height);
        throw_if_failure(status1, "Parsing error in scene constructor (resolution)");
        
        const int ret = f.scanf_count("camera position:(%lf,%lf,%lf) direction:(%lf,%lf,%lf) rightdir:(%lf,%lf,%lf) fov_width:%lf distance:%lf focal_distance:%lf aperture:%lf\n",
            posx, posy, posz, dx, dy, dz, rdx, rdy, rdz, fovw, dist, focl, apr);
        rt::vector cam_pos(posx, posy, posz);
        rt::vector cam_dir(dx, dy, dz);
        rt::vector cam_right_dir(rdx, rdy, rdz);

        if (ret < 11) {
            throw std::runtime_error("Parsing error in scene constructor (camera)");
        }
        else if (ret == 11) {
            // Focal length and aperture omitted
            depth_of_field_enabled = false;
        }

        const real fovh = fovw * static_cast<real>(height) / static_cast<real>(width);

        camera cam = (depth_of_field_enabled) ?
            camera(cam_pos, cam_dir, cam_right_dir,
                fovw, fovh, dist, width, height, focl, apr)
            :
            camera(cam_pos, cam_dir, cam_right_dir,
                fovw, fovh, dist, width, height);

        bool background_color_is_set   = false;
        bool background_texture_is_set = false;
        const std::size_t position_bg = f.position();
        rt::color background_color;
        texture background_texture;

        // Setting up the background_color (optional)
        double r, g, b;
        const exit_status status_background = f.scanf("background_color %lf %lf %lf\n", r, g, b);
        if (status_background == exit_status::Failure) {
            f.rewind(position_bg);
        }
        else {
            background_color = rt::color(r, g, b);
            background_color_is_set = true;
        }

        // Setting up the background texture (also optional)
        char bg_tfile_name[513];
        double rx, ry, rz, inverse_gamma;
        inverse_gamma = 1.0_r;
        const exit_status status_background2 = f.scanf("background_texture %512s rotate_x:%lf rotate_y:%lf rotate_z:%lf gamma:%lf\n",
            bg_tfile_name, rx, ry, rz, inverse_gamma);

        // Neither background color nor texture
        if (status_background2 == exit_status::Failure){
            if (not background_color_is_set)
                throw std::runtime_error("Parsing error in scene constructor (background)");
        }
        else if (std::abs(rx) > 2.0_r * PI || std::abs(ry) > 2.0_r * PI || std::abs(rz) > 2.0_r * PI) {
            throw std::runtime_error("Incorrect background texture angles");
        }
        else {
            const std::string bg_tfile_name_short = std::filesystem::path(bg_tfile_name).filename().generic_string();

            if (rx < 0) rx += 2.0_r * PI;
            if (ry < 0) ry += 2.0_r * PI;
            if (rz < 0) rz += 2.0_r * PI;

            printf("Parsing %s...", bg_tfile_name_short.c_str());
            fflush(stdout);
            bool bg_parsing_successful;
            background_texture = texture(bg_tfile_name, bg_parsing_successful);
            if (not bg_parsing_successful) {
                throw std::runtime_error("Parsing error in scene constructor (background texture parsing)");
            }
            else {
                printf("\r> %s texture loaded\n", bg_tfile_name_short.c_str());
                background_texture_is_set = true;
            }
        }
        
        unsigned int polygons_per_bounding = 0;

        // Optional
        {
            using enum exit_status;
            f.scanf("bvh: ");
            const std::size_t pos = f.position();
            if (Failure == f.scanf("polygons_per_bounding %u\n", polygons_per_bounding)) {
                f.rewind(pos);
                throw_if_failure(f.scanf("disabled\n"),
                    "Parsing error in scene constructor (BVH parameters)");
            }
        }
        
        /* Object definition */
        /*
        - Materials are defined with this syntax:
        
        material:(color:(120, 120, 120) emitted_color:(0, 0, 0) reflectivity:1 emission:0 specular_p:1.0 reflects_color:false transparency:0 scattering:0 refraction_index:1)


        - Objects can be defined in any order, with this syntax:
        
        sphere center:(-500, 0, 600) radius:120 [material]
        
        plane normal:(0, -1, 0) position:(0, 160, 0) [material]
        
        box center:(166, -200, 600) x_axis:(100, 100, -100) y_axis:(-200, 100, -100) 300 200 300 [material]

        triangle (-620, -100, 600) (-520,100,500) (-540, -200, 700) [material]

        quad (-620, -100, 600) (-520, 100, 600) (-540, -200, 600) (-500, -250, 600) [material]

        cylinder origin:(0, 0, 0) direction:(1, -1, 1) radius:100 length:300 [material]

        For boxes and cylinders, the axes do not need to be unit vectors, they will be normalized when the objects are defined.

        - We can declare a material as a variable (before the objects are declared):
        material m1 (...)
        It can then be used like:
        triangle (...) (...) (...) material:m1

        - We can specify textures to triangles and quads (for the moment).
        The texture must be loaded from a bmp file, and given a name:
        load_texture t1 file_name.bmp

        Then when a material is declared, we can add the following fields:
        material:(...) texture:(t1 (0.2, 0.8) (0.5, 0.15) (0.7, 0.65)) (3 points for a triangle, 4 for a quad)
        or
        material:m1 texture:(...)

        - An obj file can be loaded with its file name and a texture name.
        Mtl files are not yet supported, materials must be declared beforehand, with the same name as in the associated mtl file.
        A shifting vector and a scale should be provided.

        material wood_mat (...)  // appearing in wooden_table.mtl
        material metal_mat (...) // appearing in wooden_table.mtl
        load_texture wood wood_texture.bmp
        load_obj wooden_table.bmp (texture:wood shift:(0,0,0) scale:2) 

        - A line can be commented by adding a "#" and a space at the beginning of the line:
        # sphere [...]

        */

        std::vector<const object*> object_set;

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
        /* When polygons_per_bounding is different from 0, then objects that are not defined in an obj file are placed in
        the vector other_content. At the end, these objects are placed in a bounding alongside the ones generated during obj files parsing */
        std::vector<const object*> other_content;
        const bool bounding_enabled = polygons_per_bounding != 0;

        /* Parsing loop */

        while (not f.eof()) {

            // longest item is load_normal_map
            char s[18];
            if (f.scanf("%17s ", s) == exit_status::Failure)
                break;

            const std::string arg(s);

            /* Commented line */
            if (arg.starts_with("#")) {
                f.skip_line();
                continue;
            }
            
            /* Material declaration */
            if (arg == "material") {
                const std::string m_name = f.read_string(64);

                std::optional<material> m = parse_material(f, inverse_gamma);
                if (not m.has_value())
                    throw std::runtime_error("Material parsing error");
                
                material_wrapper_set.emplace_back(std::move(m.value()), m_name);
                continue;
            }
            
            /* BMP file loading */
            if (arg == "load_texture") {
                char t_name_buffer[65];
                char tfile_name[513];
                const exit_status status = f.scanf(" %64s %512s", t_name_buffer, tfile_name);
                throw_if_failure(status, "Parsing error in scene constructor (texture loading)");
                const std::string t_name(t_name_buffer);
                const std::string tfile_name_short = std::filesystem::path(tfile_name).filename().generic_string();
                
                bool parsing_successful;
                printf("Parsing %s...", tfile_name);
                fflush(stdout);
                texture_wrapper_set.emplace_back(texture(tfile_name, parsing_successful, inverse_gamma), t_name);

                if (parsing_successful) {
                    printf("\r> %s texture loaded                                \n", tfile_name_short.c_str());
                    continue;
                }
                printf("%s texture reading failed\n", tfile_name_short.c_str());
                throw std::runtime_error("Texture reading failed");
            }

            /* Normal map loading */
            if (arg == "load_normal_map") {
                char t_name_buffer[65];
                char tfile_name[513];
                const exit_status status = f.scanf(" %64s %512s", t_name_buffer, tfile_name);
                throw_if_failure(status, "Parsing error in scene constructor (normal map loading)");
                const std::string t_name(t_name_buffer);
                const std::string tfile_name_short = std::filesystem::path(tfile_name).filename().generic_string();

                bool parsing_successful;
                printf("Parsing %s...", tfile_name);
                fflush(stdout);
                normal_map_wrapper_set.emplace_back(normal_map(tfile_name, parsing_successful), t_name);

                if (parsing_successful) {
                    printf("\r> %s normal map loaded                                \n", tfile_name_short.c_str());
                    continue;
                }
                printf("%s normal map reading failed\n", tfile_name_short.c_str());
                throw std::runtime_error("Normal map reading failed");
            }

            /* Objects declaration */
            
            using enum object_type;

            if (arg == "sphere") {
                /* center:(-500, 0, 600) radius:120 [material] */
                double posx, posy, posz, r;
                const exit_status status = f.scanf("center:(%lf,%lf,%lf) radius:%lf material:", posx, posy, posz, r);
                throw_if_failure(status, "Parsing error in scene constructor (sphere declaration)");

                const rt::vector position(posx, posy, posz);

                const std::optional<unsigned int> m_index = get_material(f, material_wrapper_set, inverse_gamma);

                if (not m_index.has_value())
                    throw std::runtime_error("Material definition error");
                
                const sphere* sph = nullptr;

                std::optional<texture_info> info = parse_texture_info(f, texture_wrapper_set, normal_map_wrapper_set, Sphere);
                if (info.has_value()) {
                    std::vector<real>& info_vect = info.value().uv_coordinates;
                    const rt::vector forward(info_vect[0], info_vect[1], info_vect[2]);
                    const rt::vector right  (info_vect[3], info_vect[4], info_vect[5]);
                    info_vect.clear();

                    sph = new sphere(position, r, m_index.value(), texture_info_set.size(), forward, right);
                    texture_info_set.push_back(info.value());
                }
                else {
                    sph = new sphere(position, r, m_index.value());
                }

                object_set.push_back(sph);
                if (bounding_enabled)
                    other_content.push_back(sph);

                continue;                 
            }
            if (arg == "plane") {
                /* normal:(0, -1, 0) position:(0, 160, 0) [material] */
                double nx, ny, nz, px, py, pz;
                const exit_status status = f.scanf("normal:(%lf,%lf,%lf) position:(%lf,%lf,%lf) material:",
                    nx, ny, nz, px, py, pz);
                throw_if_failure(status, "Parsing error in scene constructor (plane declaration)");
                rt::vector n(nx, ny, nz);
                rt::vector p(px, py, pz);

                const std::optional<unsigned int> m_index = get_material(f, material_wrapper_set, inverse_gamma);
                if (not m_index.has_value())
                    throw std::runtime_error("Material definition error");
                
                const plane* pln = nullptr;

                std::optional<texture_info> info = parse_texture_info(f, texture_wrapper_set, normal_map_wrapper_set, Plane);
                if (info.has_value()) {

                    std::vector<real>& info_vect = info.value().uv_coordinates;
                    const rt::vector right(info_vect[0], info_vect[1], info_vect[2]);
                    const real scale = info_vect[3];
                    info_vect.clear();

                    pln = new plane(n.x, n.y, n.z, p, m_index.value(), texture_info_set.size(), right, scale);
                    texture_info_set.push_back(info.value());
                }
                else {
                    pln = new plane(n.x, n.y, n.z, p, m_index.value());
                }

                object_set.push_back(pln);
                if (bounding_enabled)
                    other_content.push_back(pln);

                continue;
            }
            if (arg == "box") {
                /* center:(166, -200, 600) x_axis:(100, 100, -100) y_axis:(-200, 100, -100) 300 200 300 */
                double cx, cy, cz, n1x, n1y, n1z, n2x, n2y, n2z, lx, ly, lz;
                const exit_status status = f.scanf("center:(%lf,%lf,%lf) x_axis:(%lf,%lf,%lf) y_axis:(%lf,%lf,%lf) %lf %lf %lf material:",
                    cx, cy, cz, n1x, n1y, n1z, n2x, n2y, n2z, lx, ly, lz);
                throw_if_failure(status, "Parsing error in scene constructor (box declaration)");
                rt::vector c(cx, cy, cz);
                rt::vector n1(n1x, n1y, n1z);
                rt::vector n2(n2x, n2y, n2z);

                const std::optional<unsigned int> m_index = get_material(f, material_wrapper_set, inverse_gamma);
                if (not m_index.has_value())
                    throw std::runtime_error("Material definition error");
                
                const box* bx = new box(c, n1.unit(), n2.unit(), lx, ly, lz, m_index.value());
                object_set.push_back(bx);
                    
                if (bounding_enabled)
                    other_content.push_back(bx);
                
                continue;
            }
            if (arg == "triangle") {
                /* (-620, -100, 600) (-520, 100, 500) (-540, -200, 700) [material] */
                double v0x, v0y, v0z, v1x, v1y, v1z, v2x, v2y, v2z;
                const exit_status status = f.scanf("(%lf,%lf,%lf) (%lf,%lf,%lf) (%lf,%lf,%lf) material:",
                    v0x, v0y, v0z,
                    v1x, v1y, v1z,
                    v2x, v2y, v2z);
                throw_if_failure(status, "Parsing error in scene constructor (triangle declaration)");
                rt::vector v0(v0x, v0y, v0z);
                rt::vector v1(v1x, v1y, v1z);
                rt::vector v2(v2x, v2y, v2z);

                const std::optional<unsigned int> m_index = get_material(f, material_wrapper_set, inverse_gamma);
                if (not m_index.has_value())
                    throw std::runtime_error("Material definition error");

                std::optional<texture_info> info = parse_texture_info(f, texture_wrapper_set, normal_map_wrapper_set, Triangle);
                const bool normal_mapping = info.has_value() && info.value().has_normal_information();
                
                const triangle* tr = normal_mapping ?
                      new triangle(v0, v1, v2, m_index.value(), texture_info_set.size(), normal_mapping, info.value())
                    : new triangle(v0, v1, v2, m_index.value(), (info.has_value() ? texture_info_set.size() : EMPTY_INDEX));
                object_set.push_back(tr);
                if (info.has_value())
                    texture_info_set.push_back(info.value());
                        
                if (bounding_enabled)
                    other_content.push_back(tr);
                
                continue;
            }
            if (arg == "quad") {
                /* (-620, -100, 600) (-520, 100, 600) (-540, -200, 600) (-500, -250, 600) [material] */
                double v0x, v0y, v0z, v1x, v1y, v1z, v2x, v2y, v2z, v3x, v3y, v3z;
                const exit_status status = f.scanf("(%lf,%lf,%lf) (%lf,%lf,%lf) (%lf,%lf,%lf) (%lf,%lf,%lf) material:",
                    v0x, v0y, v0z,
                    v1x, v1y, v1z,
                    v2x, v2y, v2z,
                    v3x, v3y, v3z);
                throw_if_failure(status, "Parsing error in scene constructor (quad declaration)");
                rt::vector v0(v0x, v0y, v0z);
                rt::vector v1(v1x, v1y, v1z);
                rt::vector v2(v2x, v2y, v2z);
                rt::vector v3(v3x, v3y, v3z);

                const std::optional<unsigned int> m_index = get_material(f, material_wrapper_set, inverse_gamma);

                if (not m_index.has_value())
                    throw std::runtime_error("Material definition error");
                
                std::optional<texture_info> info = parse_texture_info(f, texture_wrapper_set, normal_map_wrapper_set, Quad);
                const bool normal_mapping = info.has_value() && info.value().has_normal_information();
                const quad* q = normal_mapping ?
                      new quad(v0, v1, v2, v3, m_index.value(), texture_info_set.size(), normal_mapping, info.value())
                    : new quad(v0, v1, v2, v3, m_index.value(), (info.has_value() ? texture_info_set.size() : EMPTY_INDEX));
                object_set.push_back(q);
                if (info.has_value())
                    texture_info_set.push_back(info.value());
                        
                if (bounding_enabled)
                    other_content.push_back(q);

                continue;
            }
            if (arg == "cylinder") {
                /* origin:(0, 0, 0) direction:(1, -1, 1) radius:100 length:300 [material] */
                double px, py, pz, d_x, d_y, d_z;
                double r, l;
                const exit_status status = f.scanf("origin:(%lf,%lf,%lf) direction:(%lf,%lf,%lf) radius:%lf length:%lf material:",
                    px, py, pz, d_x, d_y, d_z, r, l);
                throw_if_failure(status, "Parsing error in scene constructor (cylinder declaration)");
                rt::vector p(px, py, pz);
                rt::vector d(d_x, d_y, d_z);

                const std::optional<unsigned int> m_index = get_material(f, material_wrapper_set, inverse_gamma);

                if (not m_index.has_value())
                    throw std::runtime_error("Material definition error");
                
                const cylinder* cyl = new cylinder(p, d.unit(), r, l, m_index.value());
                object_set.push_back(cyl);
                    
                if (bounding_enabled)
                    other_content.push_back(cyl);
                
                continue;
            }

            /* Obj file parsing */
            if (arg == "load_obj") {
                char ofile_name[513];
                char t_name[65];
                double sx = 0, sy = 0, sz = 0, scale = 1;
                const int ret = f.scanf_count(" %512s (texture:%64s shift:(%lf,%lf,%lf) scale:%lf)\n",
                    ofile_name, t_name, sx, sy, sz, scale);

                if (ret != 1 && ret != 6)
                    throw std::runtime_error("Parsing error in scene constructor (obj file loading)");

                rt::vector shift(sx, sy, sz);
                
                std::optional<unsigned int> t_index;

                if (ret == 6 && std::string(t_name) == "none") {

                    t_index = wrapper<texture>::find_element(texture_wrapper_set, t_name);
                    if (not t_index.has_value())
                        throw std::runtime_error("Texture not found");
                }

                const bounding* output_bd;
                const exit_status status_obj =
                    parse_obj_file(ofile_name, t_index, object_set,
                        material_wrapper_set, texture_wrapper_set,
                        texture_info_set,
                        scale, shift,
                        bounding_enabled, polygons_per_bounding, output_bd, inverse_gamma);

                if (status_obj == exit_status::Failure) {
                    printf("%s obj file reading failed\n", ofile_name);
                    throw std::runtime_error("Obj file parsing error");
                }

                if (bounding_enabled)
                    bounding_set.push_back(output_bd);

                continue;
            }

            /* Parsing error */
            throw std::runtime_error(s);
        }

        f.close();

        if (bounding_enabled)
            bounding_set.push_back(new bounding(std::move(other_content)));

        // Creation of the final structures
        auto [ material_set, texture_set, normal_map_set ] = build_sets(material_wrapper_set, texture_wrapper_set, normal_map_wrapper_set);

        background_container&& background = (background_texture_is_set) ?
              background_container(std::move(background_texture), rx, ry, rz)
            : background_container(background_color);

        scene_opt.emplace(
            std::move(object_set),
            std::move(bounding_set),
            std::move(texture_set),
            std::move(normal_map_set),
            std::move(material_set),
            std::move(texture_info_set),
            std::move(background),
            std::move(cam),
            width, height,
            polygons_per_bounding,
            1.0_r / inverse_gamma
        );
    }
    catch (const std::exception& e) {
        printf("Error during scene parsing:\n");
        printf("%s\n", e.what());
        printf("Scene creation failed\n");
    }

    return scene_opt;
}