#include "headers/scene.hpp"
#include "objects/headers/object.hpp"
#include "../screen/headers/color.hpp"
#include "../auxiliary/headers/randomgen.hpp"

#include <stdio.h>

scene::scene(const std::vector<const object*>& obj_set,
    const rt::color background,
    const int width,
    const int height,
    const double distance,
    const rt::vector position,
    //const rt::vector direction,
    // -> to be replaced by an object from a new class "camera"
    const rt::vector screen_center)

    : obj_set(obj_set), background(background), width(width), height(height),
    distance(distance), position(position), //direction(direction),
    screen_center(screen_center), rg(randomgen()) {}

scene::scene(const char* file_name)
    : obj_set(object::set), rg(randomgen()) {

    FILE* file = fopen(file_name, "r");

    if (file == NULL) {
        printf("Error, file %s does not exist\n", file_name);
        return;
    }

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
    rt::color background(r, g, b);
    
    // while (!feof(file)) {
    // }

    screen_center = rt::vector(width/2, height/2, 0);
    fclose(file);
}