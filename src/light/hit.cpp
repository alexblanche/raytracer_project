#include "hit.hpp"
#include "screen/headers/color.hpp"
#include "screen/headers/vector.hpp"


// constructors
hit::hit(ray g, rt::vector p, rt::vector n, rt::color c)
{
    gen = g;
    point = p;
    normal = n;
    col = c;
}

hit::hit()
{
    gen = ray();
    point = rt::vector();
    normal = rt::vector();
    col = rt::color::BLACK;
}

// accessors
ray hit::get_ray()
{
    return gen;
}

rt::vector hit::get_point()
{
    return point;
}

rt::vector hit::get_normal()
{
    return normal;
}

rt::color hit::get_color()
{
    return col;
} // not in the table in the subject




