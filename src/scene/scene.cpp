#include "scene/scene.hpp"
#include "scene/camera.hpp"
#include "scene/objects/object.hpp"
#include "scene/objects/bounding.hpp"
#include "scene/objects/sphere.hpp"
#include "scene/objects/plane.hpp"
#include "scene/objects/box.hpp"
#include "scene/objects/triangle.hpp"
#include "scene/objects/quad.hpp"
#include "scene/objects/cylinder.hpp"

#include "screen/color.hpp"
#include "auxiliary/randomgen.hpp"

#include "scene/material/material.hpp"
#include "scene/material/texture.hpp"

#include <stdio.h>
#include <string.h>
#include <string>

#include <limits>
std::numeric_limits<double> real;
const double infinity = real.infinity();


scene::scene(const std::vector<const object*>& object_set,
    const std::vector<const bounding*>& bounding_set,
    const std::vector<const texture*>& texture_set,
    const rt::color& background,
    const int width, const int height,
    const camera& cam,
    const unsigned int triangles_per_bounding)

    : object_set(object_set), bounding_set(bounding_set), texture_set(texture_set),
    background(background), width(width), height(height),
    cam(cam), rg(randomgen()), triangles_per_bounding(triangles_per_bounding) {}



/*** Scene description parsing ***/

/* Auxiliary function: returns a material from a description file */
material parse_material(FILE* file) {
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
        return material();
    }

    if (strcmp(refl_c, "true") == 0) {
        refl_color = true;
    }

    return material(rt::color(r, g, b), rt::color(er, eg, eb), refl, em_int, spec_p, refl_color, transp, scattering, refr_i);
}

/* Auxiliary function: parses the name of a material and returns the material associated with it,
   or parses and returns a new material */
material get_material(FILE* file, const std::vector<string>& mat_names, const std::vector<material>& mat_content) {
    const long int position = ftell(file);

    const char firstchar = fgetc(file);
    if (firstchar == '(') {
        // material declaration
        return parse_material(file);
    }
    else {
        // Moving back the pointer back by one position
        fseek(file, position, SEEK_SET);

        // material variable name
        char vname[64];
        const int ret = fscanf(file, "%64s\n", vname);
        
        if (ret != 1) {
            printf("Parsing error in get_material\n");
            return material();
        }

        unsigned int vindex = -1;
        for (unsigned int i = 0; i < mat_names.size(); i++) {
            if (mat_names.at(i).compare(vname) == 0) {
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
void parse_texture_info(FILE* file, const std::vector<string>& texture_names, material& m, bool is_triangle) {
    const long int position = ftell(file);
    char keyword[8];
    const int ret = fscanf(file, "%7s", keyword);
    if (ret != 1) {
        printf("Parsing error in parse_texture_info (keyword texture)\n");
        return;
    }

    if (strcmp(keyword, "texture") == 0) {
        char t_name[65];
        double u0, v0, u1, v1, u2, v2, u3, v3;
        const int ret2 = fscanf(file, ":(%64s", t_name);
        if (ret2 != 1) {
            printf("Parsing error in parse_texture_info (texture name)\n");
            return;
        }

        unsigned int vindex = -1;
        for (unsigned int i = 0; i < texture_names.size(); i++) {
            if (texture_names.at(i).compare(t_name) == 0) {
                vindex = i;
                break;
            }
        }
        if (vindex == ((unsigned) -1)) {
            printf("Error, texture %s not found\n", t_name);
            return;
        }

        if (is_triangle) {
            const int ret3 = fscanf(file, " (%lf,%lf) (%lf,%lf) (%lf,%lf))\n",
                &u0, &v0, &u1, &v1, &u2, &v2);
            if (ret3 != 6) {
                printf("Parsing error in parse_texture_info (triangle UV-coordinates)\n");
                return;
            }

            m.set_texture(vindex, {u0, v0, u1, v1, u2, v2});
        }
        else {
            const int ret3 = fscanf(file, " (%lf,%lf) (%lf,%lf) (%lf,%lf) (%lf,%lf))\n",
                &u0, &v0, &u1, &v1, &u2, &v2, &u3, &v3);
            if (ret3 != 8) {
                printf("Parsing error in parse_texture_info (quad UV-coordinates)\n");
                return;
            }

            m.set_texture(vindex, {u0, v0, u1, v1, u2, v2, u3, v3});
        }
    }
    else {
        // No texture info, setting the position back before the keyword
        fseek(file, position, SEEK_SET);
    }
}


/** Scene description parser **/

scene::scene(const char* file_name, bool& creation_successful)
    : rg(randomgen()) {

    creation_successful = true;

    FILE* file = fopen(file_name, "r");

    if (file == NULL) {
        printf("Error, file %s not found\n", file_name);
        creation_successful = false;
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

    int ret;

    ret = fscanf(file, "resolution width:%d height:%d\n", &width, &height);
    if (ret != 2) {
        printf("Parsing error in scene constructor (resolution)\n");
        creation_successful = false;
        return;
    }
    
    ret = fscanf(file, "camera position:(%lf,%lf,%lf) direction:(%lf,%lf,%lf) rightdir:(%lf,%lf,%lf) fov_width:%lf distance:%lf\n",
        &posx, &posy, &posz, &dx, &dy, &dz, &rdx, &rdy, &rdz, &fovw, &dist);
    if (ret != 11) {
        printf("Parsing error in scene constructor (camera)\n");
        creation_successful = false;
        return;
    }

    double fovh = fovw * ((double) height) / ((double) width);
    cam = camera(rt::vector(posx, posy, posz), rt::vector(dx, dy, dz), rt::vector(rdx, rdy, rdz), fovw, fovh, dist, width, height);

    double r, g, b;
    ret = fscanf(file, "background_color %lf %lf %lf\n", &r, &g, &b);
    if (ret != 3) {
        printf("Parsing error in scene constructor (background)\n");
        creation_successful = false;
        return;
    }

    background = rt::color(r, g, b);

    ret = fscanf(file, "triangles_per_bounding %u\n", &triangles_per_bounding);
    if (ret != 1) {
        printf("Parsing error in scene constructor (triangles per bounding)\n");
        creation_successful = false;
        return;
    }
    
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
    Objects are automatically stored in object::set.

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

    - A line can be commented by adding a "#" and a space at the beginning of the line:
    # sphere [...]

    */

    /* Material storage */
    /* The name of the material is stored at index i of mat_names, and the associated material at index i of mat_content */
    std::vector<string> mat_names = {"mirror", "glass"};
    std::vector<material> mat_content = {material::MIRROR, material::GLASS};


    /* Texture storage */
    /* The name of the texture is stored at index i of texture_names, and the associated texture at index i of texture_set */
    std::vector<string> texture_names;


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
                printf("Parsing error in scene constructor (material declaration)\n");
                creation_successful = false;
                break;
            }
            m_name.resize(strlen(m_name.data()));

            material m = parse_material(file);

            mat_names.push_back(m_name);
            mat_content.push_back(m);
        }
        /* BMP file loading */
        else if (strcmp(s, "load_texture") == 0) {
            std::string t_name(65, '\0');
            char tfile_name[65];
            ret = fscanf(file, " %64s %64s", (char*) t_name.data(), tfile_name);
            if (ret != 2) {
                printf("Parsing error in scene constructor (texture loading)\n");
                creation_successful = false;
                break;
            }
            t_name.resize(strlen(t_name.data()));
            
            texture_names.push_back(t_name);
            bool parsing_successful = true;
            texture_set.push_back(new texture(tfile_name, parsing_successful));
            if (parsing_successful) {
                printf("%s texture loaded\n", tfile_name);
            }
            else {
                printf("%s texture reading failed\n", tfile_name);
                creation_successful = false;
                break;
            }
        }

        /* Objects declaration */
        
        else if (strcmp(s, "sphere") == 0) {
            /* center:(-500, 0, 600) radius:120 [material] */
            double x, y, z, r;
            ret = fscanf(file, "center:(%lf,%lf,%lf) radius:%lf material:",
                &x, &y, &z, &r);
            if (ret != 4) {
                printf("Parsing error in scene constructor (sphere declaration)\n");
                creation_successful = false;
                break;
            }
            material m = get_material(file, mat_names, mat_content);
            object_set.push_back(new sphere(rt::vector(x, y, z), r, m));
        }
        else if (strcmp(s, "plane") == 0) {
            /* normal:(0, -1, 0) position:(0, 160, 0) [material] */
            double nx, ny, nz, px, py, pz;
            ret = fscanf(file, "normal:(%lf,%lf,%lf) position:(%lf,%lf,%lf) material:",
                &nx, &ny, &nz,
                &px, &py, &pz);
            if (ret != 6) {
                printf("Parsing error in scene constructor (plane declaration)\n");
                creation_successful = false;
                break;
            }
            material m = get_material(file, mat_names, mat_content);
            object_set.push_back(new plane(nx, ny, nz, rt::vector(px, py, pz), m));
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
                printf("Parsing error in scene constructor (box declaration)\n");
                creation_successful = false;
                break;
            }
            material m = get_material(file, mat_names, mat_content);
            object_set.push_back(new box(rt::vector(cx, cy, cz),
                rt::vector(n1x, n1y, n1z).unit(),
                rt::vector(n2x, n2y, n2z).unit(),
                l1, l2, l3, m));
        }
        else if (strcmp(s, "triangle") == 0) {
            /* (-620, -100, 600) (-520, 100, 500) (-540, -200, 700) [material] */
            double x0, y0, z0, x1, y1, z1, x2, y2, z2;
            ret = fscanf(file, "(%lf,%lf,%lf) (%lf,%lf,%lf) (%lf,%lf,%lf) material:",
                &x0, &y0, &z0,
                &x1, &y1, &z1,
                &x2, &y2, &z2);
            if (ret != 9) {
                printf("Parsing error in scene constructor (triangle declaration)\n");
                creation_successful = false;
                break;
            }
            material m = get_material(file, mat_names, mat_content);
            parse_texture_info(file, texture_names, m, true);
            object_set.push_back(new triangle(rt::vector(x0, y0, z0),
                rt::vector(x1, y1, z1),
                rt::vector(x2, y2, z2),
                m));
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
                printf("Parsing error in scene constructor (quad declaration)\n");
                creation_successful = false;
                break;
            }
            material m = get_material(file, mat_names, mat_content);
            parse_texture_info(file, texture_names, m, false);
            object_set.push_back(new quad(rt::vector(x0, y0, z0),
                rt::vector(x1, y1, z1),
                rt::vector(x2, y2, z2),
                rt::vector(x3, y3, z3),
                m));
        }
        else if (strcmp(s, "cylinder") == 0) {
            /* origin:(0, 0, 0) direction:(1, -1, 1) radius:100 length:300 [material] */
            double x0, y0, z0, dx, dy, dz, r, l;
            ret = fscanf(file, "origin:(%lf,%lf,%lf) direction:(%lf,%lf,%lf) radius:%lf length:%lf material:",
                &x0, &y0, &z0,
                &dx, &dy, &dz,
                &r, &l);
            if (ret != 8) {
                printf("Parsing error in scene constructor (cylinder declaration)\n");
                creation_successful = false;
                break;
            }
            material m = get_material(file, mat_names, mat_content);
            object_set.push_back(new cylinder(rt::vector(x0, y0, z0), rt::vector(dx, dy, dz).unit(),
                r, l, m));
        }

        /* Parsing error */

        else {
            printf("Parsing error: %s\n", s);
            creation_successful = false;
            break;
        }
    }
    
    fclose(file);
}

/*********************************************************************/

/*** Ray-scene intersection ***/

/* Linear search through the objects of the scene */
hit scene::find_closest_object(const ray& r) const {
    
    double distance_to_closest = infinity;
    unsigned int closest_obj_index = -1;

    // Looking for the closest object
    for (unsigned int i = 0; i < object_set.size(); i++) {
        
        // We do not test the intersection with the object the rays is cast from
        const double d = object_set.at(i)->measure_distance(r);
        
        /* d is the distance between the origin of the ray and the
           intersection point with the object */

        if (d < distance_to_closest && d > 0.000001) {
            distance_to_closest = d;
            closest_obj_index = i;
        }
    }

    if (closest_obj_index == ((unsigned int) -1)) {
        return hit();
    }
    else {
        return object_set.at(closest_obj_index)->compute_intersection(r, distance_to_closest);
    }
}

/* Tree-search through the bounding boxes */
hit scene::find_closest_object_bounding(const ray& r) const {
    /* For all the bounding boxes in bounding::set, we do the following:
       If the bounding box is terminal, look for the object of minimum distance.
       If it is internal, if the ray intersects the box, add its children to the bounding stack.
       Then apply the same algorithm to the bounding stack, until it is empty.
       Finally, compute the hit associated with the object of minimum distance.
     */

    double distance_to_closest = infinity;
    const object* closest_obj;
    std::stack<const bounding*> bounding_stack;

    /* Pass through the set of first-level bounding boxes */
    for (unsigned int i = 0; i < bounding_set.size(); i++) {
        bounding_set.at(i)->check_box(r, distance_to_closest, closest_obj, bounding_stack);
    }

    /* In order to avoid pushing and then immediately popping an element from bounding_stack,
       we store the last element of bd->children in next_bounding.
       The boolean bd_stored indicates whether we should pop an element, or if one is currently
       stored.
     */
    const bounding* next_bounding;
    bool bd_stored = false;

    /* Apply the same to the bounding box stack */
    while (bd_stored || (not bounding_stack.empty())) {

        const bounding* bd = bd_stored ? next_bounding : bounding_stack.top();
        if (not bd_stored) {
            bounding_stack.pop();
        }
        
        bd->check_box_next(r, distance_to_closest, closest_obj, bounding_stack,
            bd_stored, next_bounding);
    }

    /* Finally, return the hit corresponding to the closest object intersected by the ray */
    if (closest_obj != NULL) {
        return closest_obj->compute_intersection(r, distance_to_closest);
    }
    else {
        return hit();
    }
}