#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <vector>
#include "headers/hit.hpp"
#include "headers/light.hpp"
#include "../objects/headers/sphere.hpp"
#include "../objects/headers/plane.hpp"
#include "../screen/headers/color.hpp"


using namespace std;

light::light(const rt::vector& p, const rt::color& c) {
    pos = p;
    col = c;
}

light::light() {
    pos = rt::vector();
    col = rt::color::WHITE;
}

rt::vector light::get_position() const {
    return pos;
}

rt::color light::get_color() const {
    return col;
}

/* Before modfications:
rt::color light::apply(hit r) {
  rt::color hit_color = r.get_color();
  double cos_hit = (r.get_normal()|(r.get_point()-pos));

  rt::color cf = rt::color(col.get_red()*col.get_alpha()*cos_hit+hit_color.get_red()*hit_color.get_alpha(),
		col.get_green()*col.get_alpha()*cos_hit+hit_color.get_green()*hit_color.get_alpha(),
		col.get_blue()*col.get_alpha()*cos_hit+hit_color.get_blue()*hit_color.get_alpha(),
		col.get_alpha()*hit_color.get_alpha());
  return cf;
*/

rt::color light::apply(hit h) {

    rt::color hit_color = h.get_color();
    double cos_hit = ((h.get_normal()).unit()|(h.get_point()-pos).unit());
    if (cos_hit < 0) {cos_hit = 0;}; // The point is on the far side of the sphere

    //unsigned char r = col.get_red()  */*col.get_alpha()*/cos_hit + hit_color.get_red();//*hit_color.get_alpha(),
    //unsigned char g = col.get_green()*/*col.get_alpha()*/cos_hit + hit_color.get_green();//*hit_color.get_alpha(),
    //unsigned char b = col.get_blue() */*col.get_alpha()*/cos_hit + hit_color.get_blue();//*hit_color.get_alpha(),

    //unsigned char r = (col.get_red()+hit_color.get_red())  *cos_hit /2 ;
    //unsigned char g = (col.get_green()+hit_color.get_green())  *cos_hit /2 ;
    //unsigned char b = (col.get_blue()+hit_color.get_blue())  *cos_hit /2 ;

    //unsigned char a = col.get_alpha()*hit_color.get_alpha();

    //if (r>255){r = 255;};
    //if (g>255){g = 255;};
    //if (b>255){b = 255;};


    unsigned char r = (col.get_red() * hit_color.get_red()) * cos_hit / 255 ;
    unsigned char g = (col.get_green() * hit_color.get_green()) * cos_hit / 255 ;
    unsigned char b = (col.get_blue() * hit_color.get_blue()) * cos_hit / 255 ;
    /* Application of a light on a surface
    The formula is:
    (r1,g1,b1) -> (r2,g2,b2) = (r1*r2/255, g1*g2/255, b1*b2/255) */

    rt::color cf = rt::color(r,g,b);
    return cf;
}


rt::color light::apply2(hit h, vector<sphere> s) { //, vector<plane> p)

    rt::vector to_the_light = (pos-h.get_point());
    double dist = to_the_light.norm();

    ray reflected_ray(h.get_point(), to_the_light.unit(), rt::color::WHITE);

    // Seeking for an intersection with a sphere or a plane
    double d;

    for (unsigned int i=0; i<s.size(); i++) {
        d = (s.at(i)).send(reflected_ray);
        if (d <= dist) {
            return rt::color::BLACK;
        }
    }

    /*for (unsigned int i=0; i<p.size(); i++)
      {
        d = (p.at(i)).send(reflected_ray);
        if (d <= dist)
        {
            return rt::color::BLACK;
        };
    };*/
    // We simplify the problem
    // This verification was a bit useless because we don't have finite planes

    rt::color hit_color = h.get_color();
    double cos_hit = ((h.get_normal()).unit()|(h.get_point()-pos).unit());
    if (cos_hit < 0) {cos_hit = 0;};

    unsigned char r = (col.get_red() * hit_color.get_red()) * cos_hit / 255 ;
    unsigned char g = (col.get_green() * hit_color.get_green()) * cos_hit / 255 ;
    unsigned char b = (col.get_blue() * hit_color.get_blue()) * cos_hit / 255 ;

    return rt::color(r,g,b);
}
