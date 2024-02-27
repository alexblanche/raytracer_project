#include "headers/scene.hpp"
#include "objects/headers/object.hpp"
#include "objects/headers/sphere.hpp"
#include "objects/headers/plane.hpp"
#include "objects/headers/box.hpp"
#include "objects/headers/triangle.hpp"
#include "objects/headers/quad.hpp"
#include "../screen/headers/color.hpp"
#include "../auxiliary/headers/randomgen.hpp"

#include <stdio.h>
#include <string.h>

scene::scene(const rt::color background,
    const int width,
    const int height,
    const double distance,
    const rt::vector position,
    //const rt::vector direction,
    // -> to be replaced by an object from a new class "camera"
    const rt::vector screen_center,
    const unsigned int triangles_per_bounding)

    : background(background), width(width), height(height),
    distance(distance), position(position), //direction(direction),
    screen_center(screen_center), rg(randomgen()),
    triangles_per_bounding(triangles_per_bounding) {}

/* Auxiliary function that returns a material from a description file */
material parse_materials(FILE* file) {
    /* material:(color:(120,120,120) emitted_color:(0,0,0) reflectivity:1 emission:0 specular_p:1.0 reflects_color:false) */
    double r, g, b, er, eg, eb, refl, em_int, spec_p, transp, scattering, refr_i;
    bool refl_color = false;
    char refl_c[4];

    fscanf(file, "material:(color:(%lf,%lf,%lf) emitted_color:(%lf,%lf,%lf) reflectivity:%lf emission:%lf specular_p:%lf reflects_color:%s transparency:%lf scattering:%lf refraction_index:%lf)\n", 
        &r, &g, &b, &er, &eg, &eb, &refl, &em_int, &spec_p, refl_c, &transp, &scattering, &refr_i);

    if (strcmp(refl_c, "true") == 0) {
        refl_color = true;
    }

    return material(rt::color(r, g, b), rt::color(er, eg, eb), refl, em_int, spec_p, refl_color, transp, scattering, refr_i);
}

scene::scene(const char* file_name)
    : rg(randomgen()) {

    FILE* file = fopen(file_name, "r");

    if (file == NULL) {
        printf("Error, file %s does not exist\n", file_name);
        return;
    }

    /* Parameters definition

    Example:
    width 1366
    height 768
    distance 400
    position 0 0 0
    background 190 235 255
    triangles_per_bounding 10
    (specifying 0 will deactivate the bounding generation)
    */

    fscanf(file, "width %d\n", &width);
    fscanf(file, "height %d\n", &height);
    fscanf(file, "distance %lf\n", &distance);

    double posx = 0;
    double posy = 0;
    double posz = 0;
    fscanf(file, "position %lf %lf %lf\n", &posx, &posy, &posz);
    position = rt::vector(posx, posy, posz);

    double r = 0;
    double g = 0;
    double b = 0;
    fscanf(file, "background %lf %lf %lf\n", &r, &g, &b);
    background = rt::color(r, g, b);

    fscanf(file, "triangles_per_bounding %u\n", &triangles_per_bounding);
    
    screen_center = rt::vector(width/2, height/2, 0);
    
    /* Object definition */
    /*
    Materials are defined with this syntax:
    
    material:(color:(120,120,120) emitted_color:(0,0,0) reflectivity:1 emission:0 specular_p:1.0 reflects_color:false)


    Objects can be defined in any order, with this syntax:
    
    sphere center:(-500,0,600) radius:120 [material]
    
    plane normal:(0,-1,0) position:(0,160,0) [material]
    
    box center:(166,-200,600) x_axis:(100,100,-100) y_axis:(-200,100,-100) 300 200 300 [material]

    triangle (-620,-100,600) (-520,100,500) (-540,-200,700) [material]

    quad (-620,-100,600) (-520,100,600) (-540,-200,600) (-500,-250,600) [material]

    For boxes, the axes do not need to be unit vectors, they will be normalized when the objects are defined.
    */

    /* Objects are automatically stored in object::set */
    while (!feof(file)) {
        char s[8];
        fscanf(file, "\n%s ", s);

        if (strcmp(s, "sphere") == 0) {
            /* center:(-500,0,600) radius:120 [material] */
            double x, y, z, r;
            fscanf(file, "center:(%lf,%lf,%lf) radius:%lf ",
                &x, &y, &z, &r);
            material m = parse_materials(file);
            new sphere(rt::vector(x, y, z), r, m);
        }
        else if (strcmp(s, "plane") == 0) {
            /* normal:(0,-1,0) position:(0, 160, 0) [material] */
            double nx, ny, nz, px, py, pz;
            fscanf(file, "normal:(%lf,%lf,%lf) position:(%lf,%lf,%lf) ",
                &nx, &ny, &nz,
                &px, &py, &pz);
            material m = parse_materials(file);
            new plane(nx, ny, nz, rt::vector(px, py, pz), m);
        }
        else if (strcmp(s, "box") == 0) {
            /* center:(166,-200,600) x_axis:(100,100,-100) y_axis:(-200,100,-100) 300 200 300 */
            double cx, cy, cz, n1x, n1y, n1z, n2x, n2y, n2z, l1, l2, l3;
            fscanf(file, "center:(%lf,%lf,%lf) x_axis:(%lf,%lf,%lf) y_axis:(%lf,%lf,%lf) %lf %lf %lf ",
                &cx, &cy, &cz,
                &n1x, &n1y, &n1z,
                &n2x, &n2y, &n2z,
                &l1, &l2, &l3);
            material m = parse_materials(file);
            new box(rt::vector(cx, cy, cz), rt::vector(n1x, n1y, n1z).unit(), rt::vector(n2x, n2y, n2z).unit(),
                l1, l2, l3, m);
        }
        else if (strcmp(s, "triangle") == 0) {
            /* (-620,-100,600) (-520,100,500) (-540,-200,700) [material] */
            double x0, y0, z0, x1, y1, z1, x2, y2, z2;
            fscanf(file, "(%lf,%lf,%lf) (%lf,%lf,%lf) (%lf,%lf,%lf) ",
                &x0, &y0, &z0,
                &x1, &y1, &z1,
                &x2, &y2, &z2);
            material m = parse_materials(file);
            new triangle(rt::vector(x0, y0, z0), rt::vector(x1, y1, z1), rt::vector(x2, y2, z2), m);
        }
        else if (strcmp(s, "quad") == 0) {
            /* (-620,-100,600) (-520,100,600) (-540,-200,600) (-500,-250,600) [material] */
            double x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3;
            fscanf(file, "(%lf,%lf,%lf) (%lf,%lf,%lf) (%lf,%lf,%lf) (%lf,%lf,%lf) ",
                &x0, &y0, &z0,
                &x1, &y1, &z1,
                &x2, &y2, &z2,
                &x3, &y3, &z3);
            material m = parse_materials(file);
            new quad(rt::vector(x0, y0, z0), rt::vector(x1, y1, z1), rt::vector(x2, y2, z2), rt::vector(x3, y3, z3), m);
        }
        else {
            printf("Parsing error: %s\n", s);
            fclose(file);
        }
    }
    
    fclose(file);
}