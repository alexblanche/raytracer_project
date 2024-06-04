#include "file_readers/scene_parser.hpp"

#include "file_readers/obj_parser.hpp"

#include <stdio.h>
#include <string.h>
#include <string>

#include "scene/objects/bounding.hpp"
#include "scene/objects/sphere.hpp"
#include "scene/objects/plane.hpp"
#include "scene/objects/box.hpp"
#include "scene/objects/triangle.hpp"
#include "scene/objects/quad.hpp"
#include "scene/objects/cylinder.hpp"

#include <stdexcept>

#include "file_readers/parsing_wrappers.hpp"


/*** Scene description parsing ***/

/* Auxiliary function: returns a material from a description file */
std::optional<material> parse_material(FILE* file) {
    /* color:(120, 120, 120) emitted_color:(0, 0, 0) reflectivity:1 emission:0
        specular_p:1.0 reflects_color:false transparency:0.5 scattering:0 refraction_index:1.2)

        See the file structure below.    
    */

    double r, g, b, er, eg, eb, refl, em_int, spec_p, transp, scattering, refr_i;
    bool refl_color = false;
    char refl_c[6];

    const int ret = fscanf(file, "color:(%lf,%lf,%lf) emitted_color:(%lf,%lf,%lf) reflectivity:%lf emission:%lf specular_p:%lf reflects_color:%5s transparency:%lf scattering:%lf refraction_index:%lf)", 
        &r, &g, &b, &er, &eg, &eb, &refl, &em_int, &spec_p, refl_c, &transp, &scattering, &refr_i);

    if (ret != 13) {
        printf("Parsing error in parse_material\n");
        return std::nullopt;
    }

    if (strcmp(refl_c, "true") == 0) {
        refl_color = true;
    }

    return material(rt::color(r, g, b), rt::color(er, eg, eb), refl, em_int, spec_p, refl_color, transp, scattering, refr_i);
}


/* Auxiliary function: parses the name of a material and returns its index in material_set,
   or parses a new material, stores it in material_set and returns its index */
std::optional<size_t> get_material(FILE* file, std::vector<wrapper<material>>& material_wrapper_set) {
    const long int position = ftell(file);

    const char firstchar = fgetc(file);
    if (firstchar == '(') {
        // material declaration
        std::optional<material> m = parse_material(file);

        if (m.has_value()) {
            material_wrapper_set.emplace_back(std::move(m.value()));
            return material_wrapper_set[material_wrapper_set.size()-1].index;
        }
        else {
            return std::nullopt;
        }
        
    }
    else {
        // Moving back the pointer back by one position
        fseek(file, position, SEEK_SET);

        // material variable name
        char vname[65];
        const int ret = fscanf(file, "%64s\n", vname);
        
        if (ret != 1) {
            printf("Parsing error in get_material\n");
            return std::nullopt;
        }

        std::optional<size_t> vindex = std::nullopt;
        
        for (wrapper<material> const& mat_wrap : material_wrapper_set) {
            if (mat_wrap.name.has_value() && mat_wrap.name.value().compare(vname) == 0) {
                vindex = mat_wrap.index;
                break;
            }
        }

        if (vindex.has_value()) {
            return vindex;
        }
        else {
            printf("Error, material %s not found.\n", vname);
            return std::nullopt;
        }
    }
}

/* Auxiliary function: parses the texture information after a material, and if there is one,
   returns the associated texture_info
   is_triangle is true if the object is a triangle (in this case, 3 uv points are parsed),
   and false if it is a quad (4 uv points are parsed) */
std::optional<texture_info> parse_texture_info(FILE* file,
    const std::vector<wrapper<texture>>& texture_wrapper_set, const bool is_triangle) {

    const long int position = ftell(file);
    char keyword[8];
    const int ret = fscanf(file, "%7s", keyword);
    if (ret != 1) {
        printf("Parsing error in parse_texture_info (keyword texture)\n");
        return std::nullopt;
    }

    if (strcmp(keyword, "texture") == 0) {
        char t_name[65];
        double u0, v0, u1, v1, u2, v2, u3, v3;
        const int ret2 = fscanf(file, ":(%64s", t_name);
        if (ret2 != 1) {
            printf("Parsing error in parse_texture_info (texture name)\n");
            return std::nullopt;
        }

        std::optional<size_t> vindex = std::nullopt;
        for (wrapper<texture> const& txt_wrap : texture_wrapper_set) {
            if (txt_wrap.name.has_value() && txt_wrap.name.value().compare(t_name) == 0) {
                vindex = txt_wrap.index;
                break;
            }
        }

        if (not vindex.has_value()) {
            printf("Error, texture %s not found\n", t_name);
            return std::nullopt;
        }

        if (is_triangle) {
            const int ret3 = fscanf(file, " (%lf,%lf) (%lf,%lf) (%lf,%lf))\n",
                &u0, &v0, &u1, &v1, &u2, &v2);
            if (ret3 != 6) {
                printf("Parsing error in parse_texture_info (triangle UV-coordinates)\n");
                return std::nullopt;
            }

            return texture_info(vindex.value(), {u0, v0, u1, v1, u2, v2});
        }
        else {
            const int ret3 = fscanf(file, " (%lf,%lf) (%lf,%lf) (%lf,%lf) (%lf,%lf))\n",
                &u0, &v0, &u1, &v1, &u2, &v2, &u3, &v3);
            if (ret3 != 8) {
                printf("Parsing error in parse_texture_info (quad UV-coordinates)\n");
                return std::nullopt;
            }

            return texture_info(vindex.value(), {u0, v0, u1, v1, u2, v2, u3, v3});
        }
    }
    else {
        // No texture info, setting the position back before the keyword
        fseek(file, position, SEEK_SET);
        return std::nullopt;
    }
}


/** Scene description parser **/

std::optional<scene> parse_scene_descriptor(const char* file_name) {

    FILE* file = fopen(file_name, "r");

    if (file == NULL) {
        printf("Error, file %s not found\n", file_name);
        return std::nullopt;
    }

    try {

        /* Parameters definition

        Example:

        resolution width:1366 height:768
        camera position:(0, 0, 0) direction:(0, 0, 1) rightdir:(1, 0, 0) fov_width:1000 distance:400 [focal_distance:500 aperture:100] (optional)
        background_color 190 235 255
        polygons_per_bounding 10 //specifying 0 will deactivate the bounding generation

        fov_height is generated automatically (for width/height aspect ratio)
        */
        int width, height;
        double posx, posy, posz, dx, dy, dz, rdx, rdy, rdz, fovw, dist, focl, apr;
        bool depth_of_field_enabled = true;

        int ret;
        ret = fscanf(file, "resolution width:%d height:%d\n", &width, &height);

        if (ret != 2) {
            throw std::runtime_error("Parsing error in scene constructor (resolution)");
        }
        
        ret = fscanf(file, "camera position:(%lf,%lf,%lf) direction:(%lf,%lf,%lf) rightdir:(%lf,%lf,%lf) fov_width:%lf distance:%lf focal_distance:%lf aperture:%lf\n",
            &posx, &posy, &posz, &dx, &dy, &dz, &rdx, &rdy, &rdz, &fovw, &dist, &focl, &apr);

        if (ret < 11) {
            throw std::runtime_error("Parsing error in scene constructor (camera)");
        }
        else if (ret == 11) {
            // Focal length and aperture omitted
            depth_of_field_enabled = false;
        }

        double fovh = fovw * ((double) height) / ((double) width);

        camera cam;
        
        if (depth_of_field_enabled) {
            cam = camera(rt::vector(posx, posy, posz), rt::vector(dx, dy, dz), rt::vector(rdx, rdy, rdz),
                fovw, fovh, dist, width, height, focl, apr);
        }
        else {
            cam = camera(rt::vector(posx, posy, posz), rt::vector(dx, dy, dz), rt::vector(rdx, rdy, rdz),
                fovw, fovh, dist, width, height);
        }

        double r, g, b;
        ret = fscanf(file, "background_color %lf %lf %lf\n", &r, &g, &b);
        if (ret != 3) {
            throw std::runtime_error("Parsing error in scene constructor (background)");
        }

        rt::color background = rt::color(r, g, b);
        unsigned int polygons_per_bounding = 0;

        ret = fscanf(file, "polygons_per_bounding %u\n", &polygons_per_bounding);
        if (ret != 1) {
            throw std::runtime_error("Parsing error in scene constructor (polygons per bounding)");
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
        material_wrapper_set.emplace_back(std::move(material::DIFFUSE), "diffuse");
        material_wrapper_set.emplace_back(std::move(material::MIRROR), "mirror");
        material_wrapper_set.emplace_back(std::move(material::GLASS), "glass");
        material_wrapper_set.emplace_back(std::move(material::WATER), "water");

        std::vector<wrapper<texture>> texture_wrapper_set;
        // texture copies should be avoided as much as possible
        // texture_wrapper_set.reserve(10);
        
        wrapper<material>::init();
        wrapper<texture>::init();

        std::vector<const bounding*> bounding_set;

        /* Bounding handling */
        /* When polygons_per_bounding is different from 0, then objects that are not defined in an obj file are placed in
        the vector other_content. At the end, these objects are placed in a bounding alongside the ones generated during obj files parsing */
        std::vector<const object*> other_content;
        const bool bounding_enabled = polygons_per_bounding != 0;


        /* Parsing loop */

        while (not feof(file)) {

            // longest item is load_texture
            char s[14];
            if (fscanf(file, "%13s ", s) != 1) {
                break;
            }

            /* Commented line */
            if (strcmp(s, "#") == 0) {
                char c;
                do {
                    c = fgetc(file);
                }
                while (c != '\n' && c != EOF);
                ungetc(c, file);
            }
            /* Material declaration */
            else if (strcmp(s, "material") == 0) {
                std::string m_name(65, '\0');
                ret = fscanf(file, " %64s (", (char*) m_name.data());
                if (ret != 1) {
                    throw std::runtime_error("Parsing error in scene constructor (material declaration)");
                }
                m_name.resize(strlen(m_name.data()));

                std::optional<material> m = parse_material(file);
                if (m.has_value()) {
                    material_wrapper_set.emplace_back(std::move(m.value()), m_name);
                }
                else {
                    throw std::runtime_error("Material parsing error");
                }
                
            }
            /* BMP file loading */
            else if (strcmp(s, "load_texture") == 0) {
                std::string t_name(65, '\0');
                char tfile_name[513];
                ret = fscanf(file, " %64s %512s", (char*) t_name.data(), tfile_name);
                if (ret != 2) {
                    throw std::runtime_error("Parsing error in scene constructor (texture loading)");
                }
                t_name.resize(strlen(t_name.data()));
                
                bool parsing_successful;
                texture_wrapper_set.emplace_back(texture(tfile_name, parsing_successful), t_name);

                if (parsing_successful) {
                    printf("%s texture loaded\n", tfile_name);
                    fflush(stdout);
                }
                else {
                    printf("%s texture reading failed\n", tfile_name);
                    throw std::runtime_error("Texture reading failed");
                }
            }

            /* Objects declaration */
            
            else if (strcmp(s, "sphere") == 0) {
                /* center:(-500, 0, 600) radius:120 [material] */
                double x, y, z, r;
                ret = fscanf(file, "center:(%lf,%lf,%lf) radius:%lf material:",
                    &x, &y, &z, &r);
                if (ret != 4) {
                    throw std::runtime_error("Parsing error in scene constructor (sphere declaration)");
                }
                const std::optional<size_t> m_index = get_material(file, material_wrapper_set);
                if (m_index.has_value()) {
                    const sphere* sph = new sphere(rt::vector(x, y, z), r, m_index.value());
                    object_set.push_back(sph);
                    
                    if (bounding_enabled) {
                        other_content.push_back(sph);
                    }
                }
                else {
                    throw std::runtime_error("Material definition error");
                }
                
            }
            else if (strcmp(s, "plane") == 0) {
                /* normal:(0, -1, 0) position:(0, 160, 0) [material] */
                double nx, ny, nz, px, py, pz;
                ret = fscanf(file, "normal:(%lf,%lf,%lf) position:(%lf,%lf,%lf) material:",
                    &nx, &ny, &nz,
                    &px, &py, &pz);
                if (ret != 6) {
                    throw std::runtime_error("Parsing error in scene constructor (plane declaration)");
                }
                const std::optional<size_t> m_index = get_material(file, material_wrapper_set);
                if (m_index.has_value()) {
                    const plane* pln = new plane(nx, ny, nz, rt::vector(px, py, pz), m_index.value());
                    object_set.push_back(pln);
                        
                    if (bounding_enabled) {
                        other_content.push_back(pln);
                    }
                }
                else {
                    throw std::runtime_error("Material definition error");
                }
            }
            else if (strcmp(s, "box") == 0) {
                /* center:(166, -200, 600) x_axis:(100, 100, -100) y_axis:(-200, 100, -100) 300 200 300 */
                double cx, cy, cz, n1x, n1y, n1z, n2x, n2y, n2z, l1, l2, l3;
                ret = fscanf(file, "center:(%lf,%lf,%lf) x_axis:(%lf,%lf,%lf) y_axis:(%lf,%lf,%lf) %lf %lf %lf material:",
                    &cx, &cy, &cz,
                    &n1x, &n1y, &n1z,
                    &n2x, &n2y, &n2z,
                    &l1, &l2, &l3);
                if (ret != 12) {
                    throw std::runtime_error("Parsing error in scene constructor (box declaration)");
                }
                const std::optional<size_t> m_index = get_material(file, material_wrapper_set);
                if (m_index.has_value()) {
                    const box* bx = new box(
                        rt::vector(cx, cy, cz),
                        rt::vector(n1x, n1y, n1z).unit(),
                        rt::vector(n2x, n2y, n2z).unit(),
                        l1, l2, l3, m_index.value());
                    object_set.push_back(bx);
                        
                    if (bounding_enabled) {
                        other_content.push_back(bx);
                    }
                }
                else {
                    throw std::runtime_error("Material definition error");
                }
                
            }
            else if (strcmp(s, "triangle") == 0) {
                /* (-620, -100, 600) (-520, 100, 500) (-540, -200, 700) [material] */
                double x0, y0, z0, x1, y1, z1, x2, y2, z2;
                ret = fscanf(file, "(%lf,%lf,%lf) (%lf,%lf,%lf) (%lf,%lf,%lf) material:",
                    &x0, &y0, &z0,
                    &x1, &y1, &z1,
                    &x2, &y2, &z2);
                if (ret != 9) {
                    throw std::runtime_error("Parsing error in scene constructor (triangle declaration)");
                }
                const std::optional<size_t> m_index = get_material(file, material_wrapper_set);
                const std::optional<texture_info> info = parse_texture_info(file, texture_wrapper_set, true);
                if (m_index.has_value()) {
                
                    const triangle* tr = new triangle(
                        rt::vector(x0, y0, z0),
                        rt::vector(x1, y1, z1),
                        rt::vector(x2, y2, z2),
                        m_index.value(), info);
                    object_set.push_back(tr);
                            
                    if (bounding_enabled) {
                        other_content.push_back(tr);
                    }
                }
                else {
                    throw std::runtime_error("Material definition error");
                }
                
            }
            else if (strcmp(s, "quad") == 0) {
                /* (-620, -100, 600) (-520, 100, 600) (-540, -200, 600) (-500, -250, 600) [material] */
                double x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3;
                ret = fscanf(file, "(%lf,%lf,%lf) (%lf,%lf,%lf) (%lf,%lf,%lf) (%lf,%lf,%lf) material:",
                    &x0, &y0, &z0,
                    &x1, &y1, &z1,
                    &x2, &y2, &z2,
                    &x3, &y3, &z3);
                if (ret != 12) {
                    throw std::runtime_error("Parsing error in scene constructor (quad declaration)");
                }
                const std::optional<size_t> m_index = get_material(file, material_wrapper_set);

                if (m_index.has_value()) {
                
                    const std::optional<texture_info> info = parse_texture_info(file, texture_wrapper_set, false);
                    const quad* q = new quad(
                        rt::vector(x0, y0, z0),
                        rt::vector(x1, y1, z1),
                        rt::vector(x2, y2, z2),
                        rt::vector(x3, y3, z3),
                        m_index.value(), info);
                    object_set.push_back(q);
                            
                    if (bounding_enabled) {
                        other_content.push_back(q);
                    }
                }
                else {
                    throw std::runtime_error("Material definition error");
                }
            }
            else if (strcmp(s, "cylinder") == 0) {
                /* origin:(0, 0, 0) direction:(1, -1, 1) radius:100 length:300 [material] */
                double x0, y0, z0, dx, dy, dz, r, l;
                ret = fscanf(file, "origin:(%lf,%lf,%lf) direction:(%lf,%lf,%lf) radius:%lf length:%lf material:",
                    &x0, &y0, &z0,
                    &dx, &dy, &dz,
                    &r, &l);
                if (ret != 8) {
                    throw std::runtime_error("Parsing error in scene constructor (cylinder declaration)");
                }
                const std::optional<size_t> m_index = get_material(file, material_wrapper_set);

                if (m_index.has_value()) {

                    const cylinder* cyl = new cylinder(rt::vector(x0, y0, z0), rt::vector(dx, dy, dz).unit(),
                        r, l, m_index.value());
                    object_set.push_back(cyl);
                        
                    if (bounding_enabled) {
                        other_content.push_back(cyl);
                    }

                }
                else {
                    throw std::runtime_error("Material definition error");
                }
            }

            /* Obj file parsing */
            else if (strcmp(s, "load_obj") == 0) {
                char ofile_name[513];
                char t_name[65];
                double sx, sy, sz, scale;
                ret = fscanf(file, " %512s (texture:%64s shift:(%lf,%lf,%lf) scale:%lf)\n",
                    ofile_name, t_name, &sx, &sy, &sz, &scale);

                if (ret != 6) {
                    throw std::runtime_error("Parsing error in scene constructor (obj file loading)");
                }

                
                std::optional<size_t> t_index = std::nullopt;

                if (strcmp(t_name, "none") != 0) {
                    /* Looking up the texture name in the vector of already declared texture names */
                    for (wrapper<texture> const& txt_wrap : texture_wrapper_set) {
                        if (txt_wrap.name.has_value() && txt_wrap.name.value().compare(t_name) == 0) {
                            t_index = txt_wrap.index;
                            break;
                        }
                    }
                
                    if (not t_index.has_value()) {
                        printf("Error, texture %s not found\n", t_name);
                        throw std::runtime_error("Texture not found");
                    }
                }

                const bounding* output_bd;
                const bool parsing_successful = parse_obj_file(ofile_name, t_index.value(), object_set,
                    material_wrapper_set, texture_wrapper_set,
                    scale, rt::vector(sx, sy, sz),
                    bounding_enabled, polygons_per_bounding, output_bd);

                if (not parsing_successful) {
                    printf("%s obj file reading failed\n", ofile_name);
                    throw std::runtime_error("Obj file parsing error");
                }

                if (bounding_enabled) {
                    bounding_set.push_back(output_bd);
                }
            }

            /* Parsing error */

            else {
                throw std::runtime_error(s);
            }
        }

        if (bounding_enabled) {
            bounding_set.push_back(new bounding(other_content));
        }

        if (file != NULL) {
            fclose(file);
        }

        // Creation of the final structures
        std::vector<material> material_set(wrapper<material>::counter);
        for (wrapper<material>& mat_wrap : material_wrapper_set) {
            material_set[mat_wrap.index] = std::move(mat_wrap.content);
        }
        
        std::vector<texture> texture_set(wrapper<texture>::counter);
        for (wrapper<texture>& txt_wrap : texture_wrapper_set) {
            texture_set[txt_wrap.index] = std::move(txt_wrap.content);
        }
        
        std::optional<scene> scene_opt;
        scene_opt.emplace(std::move(object_set), std::move(bounding_set), std::move(texture_set), std::move(material_set),
            background, width, height, cam, polygons_per_bounding);
        return scene_opt;

    }
    catch (const exception& e) {
        printf("Error during scene parsing:\n");
        printf("%s\n", e.what());
        
        if (file != NULL) {
            fclose(file);
        }

        return std::nullopt;
    }
}