#include "headers/scene.hpp"
#include "headers/camera.hpp"
#include "objects/headers/object.hpp"
#include "objects/headers/sphere.hpp"
#include "objects/headers/plane.hpp"
#include "objects/headers/box.hpp"
#include "objects/headers/triangle.hpp"
#include "objects/headers/quad.hpp"
#include "objects/headers/cylinder.hpp"

#include "../screen/headers/color.hpp"
#include "../auxiliary/headers/randomgen.hpp"

#include "material/headers/material.hpp"
#include "material/headers/texture.hpp"

#include <stdio.h>
#include <string.h>

scene::scene(const rt::color background,
    const int width, const int height,
    const camera& cam,
    const unsigned int triangles_per_bounding)

    : background(background), width(width), height(height),
    cam(cam), rg(randomgen()), triangles_per_bounding(triangles_per_bounding) {}



/*** Scene description parsing ***/

/* Auxiliary function: returns a material from a description file */
material parse_materials(FILE* file) {
    /* color:(120, 120, 120) emitted_color:(0, 0, 0) reflectivity:1 emission:0
        specular_p:1.0 reflects_color:false transparency:0.5 scattering:0 refraction_index:1.2)

        See the file structure below.    
     */

    double r, g, b, er, eg, eb, refl, em_int, spec_p, transp, scattering, refr_i;
    bool refl_color = false;
    char refl_c[6];

    fscanf(file, "color:(%lf,%lf,%lf) emitted_color:(%lf,%lf,%lf) reflectivity:%lf emission:%lf specular_p:%lf reflects_color:%5s transparency:%lf scattering:%lf refraction_index:%lf)", 
        &r, &g, &b, &er, &eg, &eb, &refl, &em_int, &spec_p, refl_c, &transp, &scattering, &refr_i);

    if (strcmp(refl_c, "true") == 0) {
        refl_color = true;
    }

    return material(rt::color(r, g, b), rt::color(er, eg, eb), refl, em_int, spec_p, refl_color, transp, scattering, refr_i);
}

/* Auxiliary function: parses the name of a material and returns the material associated with it,
   or parses and returns a new material */
material get_material(FILE* file, const std::vector<char*>& mat_names, const std::vector<material>& mat_content) {
    const long int position = ftell(file);

    const char firstchar = fgetc(file);
    if (firstchar == '(') {
        // material declaration
        return parse_materials(file);
    }
    else {
        // Moving back the pointer back by one position
        fseek(file, position, SEEK_SET);

        // material variable name
        char vname[64];
        fscanf(file, "%64s\n", vname);

        unsigned int vindex = -1;
        for (unsigned int i = 0; i < mat_names.size(); i++) {
            if (strcmp(mat_names.at(i), vname) == 0) {
                vindex = i;
                break;
            }
        }
        if (vindex == ((unsigned) -1)) {
            printf("Error, material %s not found.\n", vname);
            return material();
        }
        else {
            return mat_content.at(vindex);
        }
    }
}

/* Auxiliary function: parses the texture information after a material, and if there is one,
   set up the material m with said info
   is_triangle is true if the object is a triangle (in this case, 3 uv points are parsed),
   and false if it is a quad (4 uv points are parsed) */
void parse_texture_info(FILE* file, const std::vector<char*>& texture_names, material& m, bool is_triangle) {
    const long int position = ftell(file);
    char keyword[8];
    fscanf(file, "%7s", keyword);
    if (strcmp(keyword, "texture") == 0) {
        char t_name[65];
        double u0, v0, u1, v1, u2, v2, u3, v3;
        fscanf(file, ":(%64s", t_name);

        unsigned int vindex = -1;
        for (unsigned int i = 0; i < texture_names.size(); i++) {
            if (strcmp(texture_names.at(i), t_name) == 0) {
                vindex = i;
                break;
            }
        }
        if (vindex == ((unsigned) -1)) {
            printf("Error, texture %s not found.\n", t_name);
            return;
        }

        if (is_triangle) {
            fscanf(file, " (%lf,%lf) (%lf,%lf) (%lf,%lf))\n",
                &u0, &v0, &u1, &v1, &u2, &v2);
            m.set_texture(vindex, {u0, v0, u1, v1, u2, v2});
        }
        else {
            fscanf(file, " (%lf,%lf) (%lf,%lf) (%lf,%lf) (%lf,%lf))\n",
                &u0, &v0, &u1, &v1, &u2, &v2, &u3, &v3);

            m.set_texture(vindex, {u0, v0, u1, v1, u2, v2, u3, v3});
        }
    }
    else {
        // No texture info, setting the position back before the keyword
        fseek(file, position, SEEK_SET);
    }
}


/** Scene description parser **/

scene::scene(const char* file_name)
    : rg(randomgen()) {

    FILE* file = fopen(file_name, "r");

    if (file == 0) {
        printf("Error, file %s not found\n", file_name);
        return;
    }

    /* Parameters definition

    Example:

    resolution width:1366 height:768
    camera position:(0, 0, 0) direction:(0, 0, 1) rightdir:(1, 0, 0) fov_width:1000 distance:400
    background_color 190 235 255
    triangles_per_bounding 10 //specifying 0 will deactivate the bounding generation

    fov_height is generated automatically (for width/height aspect ratio)
    */
    double posx, posy, posz, dx, dy, dz, rdx, rdy, rdz, fovw, dist;

    fscanf(file, "resolution width:%d height:%d\n", &width, &height);
    fscanf(file, "camera position:(%lf,%lf,%lf) direction:(%lf,%lf,%lf) rightdir:(%lf,%lf,%lf) fov_width:%lf distance:%lf\n",
        &posx, &posy, &posz, &dx, &dy, &dz, &rdx, &rdy, &rdz, &fovw, &dist);

    double fovh = fovw * ((double) height) / ((double) width);
    cam = camera(rt::vector(posx, posy, posz), rt::vector(dx, dy, dz), rt::vector(rdx, rdy, rdz), fovw, fovh, dist, width, height);

    double r, g, b;
    fscanf(file, "background_color %lf %lf %lf\n", &r, &g, &b);
    background = rt::color(r, g, b);

    fscanf(file, "triangles_per_bounding %u\n", &triangles_per_bounding);
    
    /* Object definition */
    /*
    - Materials are defined with this syntax:
    
    material:(color:(120, 120, 120) emitted_color:(0, 0, 0) reflectivity:1 emission:0 specular_p:1.0 reflects_color:false)


    - Objects can be defined in any order, with this syntax:
    
    sphere center:(-500, 0, 600) radius:120 [material]
    
    plane normal:(0, -1, 0) position:(0, 160, 0) [material]
    
    box center:(166, -200, 600) x_axis:(100, 100, -100) y_axis:(-200, 100, -100) 300 200 300 [material]

    triangle (-620, -100, 600) (-520,100,500) (-540, -200, 700) [material]

    quad (-620, -100, 600) (-520, 100, 600) (-540, -200, 600) (-500, -250, 600) [material]

    cylinder origin:(0, 0, 0) direction:(1, -1, 1) radius:100 length:300 [material]

    For boxes and cylinders, the axes do not need to be unit vectors, they will be normalized when the objects are defined.


    - We can declare a material as a variable (before the objects are declared):
    material m1:(...)
    It can then be used like:
    triangle (...) (...) (...) material:m1

    - We can specify textures to triangles and quads (for the moment).
    The texture must be loaded from a bmp file, and given a name:
    load_texture t1 file_name.bmp

    Then when a material is declared, we can add the following fields:
    material:(...) texture:(t1 (0.2, 0.8) (0.5, 0.15) (0.7, 0.65)) (3 points for a triangle, 4 for a quad)
    or
    material:m1 texture:(...)

    */
    /* Objects are automatically stored in object::set */


    /* Material storage */

    /* Vector that stores the material variable names */
    std::vector<char*> mat_names = {(char*) "mirror", (char*) "glass"};

    /* Vector that stores the materials:
       if vname is stored at index i of vnames, the associated material is stored at index i in mats */
    std::vector<material> mat_content = {material::MIRROR, material::GLASS};


    /* Texture storage */
    /* The name of the texture is pushed to texture_names at index i, which corresponds to the index of
       the texture content automatically pushed to the static texture::set */
    std::vector<char*> texture_names;


    /* Parsing loop */

    while (not feof(file)) {

        // longest item is load_texture
        char s[14];
        if (fscanf(file, "%13s ", s) != 1) {
            break;
        }

        if (strcmp(s, "material") == 0) {
            char* m_name = new char[65];
            fscanf(file, " %64s (", m_name);
            material m = parse_materials(file);
            mat_names.push_back(m_name);
            mat_content.push_back(m);
        }
        else if (strcmp(s, "load_texture") == 0) {
            char* t_name = new char[65];
            char tfile_name[65];
            fscanf(file, " %64s %64s", t_name, tfile_name);
            texture_names.push_back(t_name);
            new texture(tfile_name);
            printf("%s texture loaded\n", tfile_name);
        }
        else if (strcmp(s, "sphere") == 0) {
            /* center:(-500, 0, 600) radius:120 [material] */
            double x, y, z, r;
            fscanf(file, "center:(%lf,%lf,%lf) radius:%lf material:",
                &x, &y, &z, &r);
            material m = get_material(file, mat_names, mat_content);
            new sphere(rt::vector(x, y, z), r, m);
        }
        else if (strcmp(s, "plane") == 0) {
            /* normal:(0, -1, 0) position:(0, 160, 0) [material] */
            double nx, ny, nz, px, py, pz;
            fscanf(file, "normal:(%lf,%lf,%lf) position:(%lf,%lf,%lf) material:",
                &nx, &ny, &nz,
                &px, &py, &pz);
            material m = get_material(file, mat_names, mat_content);
            new plane(nx, ny, nz, rt::vector(px, py, pz), m);
        }
        else if (strcmp(s, "box") == 0) {
            /* center:(166, -200, 600) x_axis:(100, 100, -100) y_axis:(-200, 100, -100) 300 200 300 */
            double cx, cy, cz, n1x, n1y, n1z, n2x, n2y, n2z, l1, l2, l3;
            fscanf(file, "center:(%lf,%lf,%lf) x_axis:(%lf,%lf,%lf) y_axis:(%lf,%lf,%lf) %lf %lf %lf material:",
                &cx, &cy, &cz,
                &n1x, &n1y, &n1z,
                &n2x, &n2y, &n2z,
                &l1, &l2, &l3);
            material m = get_material(file, mat_names, mat_content);
            new box(rt::vector(cx, cy, cz), rt::vector(n1x, n1y, n1z).unit(), rt::vector(n2x, n2y, n2z).unit(),
                l1, l2, l3, m);
        }
        else if (strcmp(s, "triangle") == 0) {
            /* (-620, -100, 600) (-520, 100, 500) (-540, -200, 700) [material] */
            double x0, y0, z0, x1, y1, z1, x2, y2, z2;
            fscanf(file, "(%lf,%lf,%lf) (%lf,%lf,%lf) (%lf,%lf,%lf) material:",
                &x0, &y0, &z0,
                &x1, &y1, &z1,
                &x2, &y2, &z2);
            material m = get_material(file, mat_names, mat_content);
            parse_texture_info(file, texture_names, m, true);
            new triangle(rt::vector(x0, y0, z0), rt::vector(x1, y1, z1), rt::vector(x2, y2, z2), m);
        }
        else if (strcmp(s, "quad") == 0) {
            /* (-620, -100, 600) (-520, 100, 600) (-540, -200, 600) (-500, -250, 600) [material] */
            double x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3;
            fscanf(file, "(%lf,%lf,%lf) (%lf,%lf,%lf) (%lf,%lf,%lf) (%lf,%lf,%lf) material:",
                &x0, &y0, &z0,
                &x1, &y1, &z1,
                &x2, &y2, &z2,
                &x3, &y3, &z3);
            material m = get_material(file, mat_names, mat_content);
            parse_texture_info(file, texture_names, m, false);
            new quad(rt::vector(x0, y0, z0), rt::vector(x1, y1, z1), rt::vector(x2, y2, z2), rt::vector(x3, y3, z3), m);
        }
        else if (strcmp(s, "cylinder") == 0) {
            /* origin:(0, 0, 0) direction:(1, -1, 1) radius:100 length:300 [material] */
            double x0, y0, z0, dx, dy, dz, r, l;
            fscanf(file, "origin:(%lf,%lf,%lf) direction:(%lf,%lf,%lf) radius:%lf length:%lf material:",
                &x0, &y0, &z0,
                &dx, &dy, &dz,
                &r, &l);
            material m = get_material(file, mat_names, mat_content);
            new cylinder(rt::vector(x0, y0, z0), rt::vector(dx, dy, dz).unit(), r, l, m);
        }
        else {
            printf("Parsing error: %s\n", s);
            break;
        }
    }

    /* Deleting the stored names */
    for (unsigned int i = 0; i < mat_names.size(); i++) {
        delete(mat_names.at(i));
    }
    for (unsigned int i = 0; i < texture_names.size(); i++) {
        delete(texture_names.at(i));
    }
    
    fclose(file);
}