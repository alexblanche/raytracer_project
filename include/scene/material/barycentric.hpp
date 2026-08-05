#pragma once

#include "parameters.hpp"

/* Structure that contains information for barycentric coordinates
   l1, l2: ST-coordinates (coordinates in object space)
   triangle_side: in the case of quad, whether the point lies in the (p0,p1,p2) triangle or the (p0,p3,p2) one */

// enum class side {
//     Triangle012, Triangle032
// };

// struct barycentric_info {
//     real l[2];
//     object_type type;
//     side triangle_side;

//     barycentric_info(real l1, real l2, object_type type, side triangle_side = side::Triangle012)
//         : l(l1, l2), type(type), triangle_side(triangle_side) {}
// };