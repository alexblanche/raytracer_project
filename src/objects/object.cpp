#include "headers/object.hpp"

/* Accessors */

rt::vector object::get_position() const {
    return position;
}

rt::color object::get_color() const {
    return color;
}