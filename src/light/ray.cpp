#include "headers/ray.hpp"

using namespace std;

ray::ray(const rt::vector& o, const rt::vector& d, const rt::color& c)
  : orig(o), dir(d), col(c) {};

ray::ray(const rt::vector& o, const rt::vector& d) {
  orig = o;
  dir = d;
  col = rt::color();
}

ray::ray() {
  orig = rt::vector();
  dir = rt::vector();
  col = rt::color();
}

rt::vector ray::get_origin() const {
  return orig;
}

rt::vector ray::get_direction() const {
  return dir;
}

rt::color ray::get_color() const {
  return col;
}

