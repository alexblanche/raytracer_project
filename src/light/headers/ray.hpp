#pragma once

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "vector.hpp"
#include "../../screen/headers/color.hpp"

using namespace std;

class ray {
  rt::vector orig;
  rt::vector dir;
  rt::color col;

  public :
    ray (const rt::vector& o,const rt::vector& d, const rt::color& c);
    ray (const rt::vector& o, const rt::vector& d);
    ray ();
    rt::vector get_origin() const;
    rt::vector get_direction() const;
    rt::color get_color() const;
}
;
