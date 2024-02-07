#include "headers/scene.hpp"
#include "objects/headers/object.hpp"
#include "../screen/headers/color.hpp"
#include "../auxiliary/headers/randomgen.hpp"

scene::scene(const std::vector<const object*>& obj_set,
    const rt::color background,
    const int width,
    const int height,
    const double distance,
    const rt::vector position,
    //const rt::vector direction,
    const rt::vector screen_center)

    : obj_set(obj_set), width(width), height(height),
    distance(distance), position(position), //direction(direction),
    screen_center(screen_center), rg(randomgen()) {}