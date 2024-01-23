#pragma once

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <vector>

#include "hit.hpp"
/*
#include "sphere.hpp"
#include "plane.hpp"
*/

using namespace std;

class light {
  rt::vector pos;
  rt::color col;

  public :
    light(const rt::vector& o,const rt::color& c);
    light();
    rt::vector get_position() const;
    rt::color get_color() const;
    rt::color apply(hit h);

    //rt::color apply2(hit h, vector<sphere>);//, vector<plane>);
}
;
