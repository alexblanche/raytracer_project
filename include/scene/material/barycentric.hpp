#pragma once

#include "parameters.hpp"

/* Structure that contains information for barycentric coordinates
   l1, l2: ST-coordinates (coordinates in object space)
   lower_triangle: true if the point lies in the lower_triangle (p0, p1, p2) of a quad */

enum class side {
    LowerTriangle, HigherTriangle
};

struct barycentric_info {
    real l[2];
    object_type type;
    side triangle_side;

    barycentric_info(real l1, real l2, object_type type, side triangle_side)
        : l(l1, l2), type(type), triangle_side(triangle_side) {}

    barycentric_info(real l1, real l2, object_type type)
        : l(l1, l2), type(type), triangle_side(side::LowerTriangle) {}
};