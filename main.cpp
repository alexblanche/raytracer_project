#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <vector>

#include <limits>
std::numeric_limits<double> real;
const double infinity = real.infinity();

#include "src/screen/headers/color.hpp"
#include "src/screen/headers/rect.hpp"
#include "src/screen/headers/image.hpp"
#include "src/screen/headers/screen.hpp"

#include "src/light/headers/vector.hpp"
#include "src/light/headers/hit.hpp"
#include "src/light/headers/light.hpp"
#include "src/light/headers/ray.hpp"

#include "src/objects/headers/object.hpp"
#include "src/objects/headers/sphere.hpp"
#include "src/objects/headers/plane.hpp"

using namespace std;

// -> To be KEPT and adapted to vector<object>

/*  Launches a ray and returns only the color of the surface hit */
rt::color launch_ray1(ray r, vector<sphere> t) {
    // We launch a ray with origin orig (the camera) and direction dir
    // The vector t is the array containing the spheres
    double d;
    double closest = infinity;
    int closest_index = 0;
    rt::color sphere_color;

    for (unsigned int i = 0; i < t.size(); i++) {
        d = (t.at(i)).send(r);
        // d is the distance between the origin of the ray and the
        // intersection point with the sphere
        if (d < closest) {
            closest = d;
            closest_index = i;
        }
    }

    if (closest != infinity) {
        return (t.at(closest_index)).get_color();
    }
    else {
        return rt::color::BLACK;
    }
}


rt::color add_col_vect(vector<rt::color> t) {
    unsigned int n = t.size();
    int r = 0;
    int g = 0;
    int b = 0;
    rt::color *c;

    for (unsigned int i = 0; i<n; i++) {
        c = &(t.at(i));

        r += c->get_red();
        g += c->get_green();
        b += c->get_blue();
    }

    if (r>255) {r = 255;}
    if (g>255) {g = 255;}
    if (b>255) {b = 255;}

    return (rt::color(r,g,b));
}
// Returns the addition of all the colors of the given vector

vector<rt::color> apply_lights(hit h, vector<light> t1) {

    unsigned int n = t1.size();
    vector<rt::color> t2(n);

    for (unsigned int i = 0 ; i < n ; i++) {
        t2.at(i) = (t1.at(i)).apply(h);
    }

    return t2;
}

/*
The formula for the addition of lights is:
(r1,g1,b1) + (r2,g2,b2) = (min(r1+r2, 255), min(g1+g2, 255), min(b1+b2, 255))
*/


vector<rt::color> apply_lights2(hit h, vector<sphere> s, vector<light> l) { //, vector<plane> p,

    unsigned int n = l.size();
    vector<rt::color> t(n);

    for (unsigned int i = 0 ; i<n ; i++) {
        t.at(i) = (l.at(i)).apply2(h,s); //,p);
    }

    return t;
}


rt::color launch_ray3(ray r, vector<sphere> s, vector<plane> p, vector<light> l) {

    hit h;
    double d;
    double closest = infinity;
    int closest_index = 0;
    bool is_sphere = true;

    // Seeking for the closest sphere
    for (unsigned int i=0; i<s.size(); i++) {

        d = s.at(i).send(r);
        if (d < closest) {
            closest = d;
            closest_index = i;
        }
    }

    // Seeking for the closest plane
    for (unsigned int i=0; i<p.size(); i++) {
        d = p.at(i).send(r);
        if (d < closest) {
            closest = d;
            closest_index = i;
            is_sphere = false;
        }
    }

    if (closest == infinity) {
        return (rt::color::BLACK); // No sphere hit, Black is returned as the color of the 'vacuum'
    }
    else {
        if (is_sphere == true) {
            h = s.at(closest_index).intersect(r, closest);
        }
        else {
            h = p.at(closest_index).intersect(r, closest);
        }

        //return (add_col_vect (apply_lights(h,l)));
        return (add_col_vect (apply_lights2(h,s/*,p*/,l)));
    }
}



/* ********************************* */
/* ********************************* */

/* ********* MAIN FUNCTION ********* */


int main(int argc, char *argv[]) {

    rt::color my_red(230, 15, 15);
    rt::color my_green(15, 230, 15);
    rt::color my_blue(15, 15, 230);
    rt::color my_white(230, 230, 230);
    rt::color my_black(15, 15, 15);
    /* Non-pure colors, to prevent the "black hole" effect when
     a red surface is under a blue spot */

    /* Orientation of the space:
    negative x on the left, positive on the right
    negative y on the top,  positive on the bottom (Be careful!!!)
    negative z behind the camera, positive in front of it
    */

    /* *************************** */

    // Spheres

    // Sphere 0
    sphere sph0(rt::vector(-400,0,1000), 240, rt::color::WHITE);
    // Sphere 1
    sphere sph1(rt::vector( 400,0,1000), 240, rt::color::WHITE);

    // Array of the spheres in the scene
    vector<sphere> sphere_set {sph0, sph1};

    /* *************************** */

    // Planes

    // Plane 0
    plane pln0(0, 1, 0, rt::vector(0, 240, 0), rt::color::WHITE);
    // Plane 1
    plane pln1(0, 0, 1, rt::vector(0, 0, 2000), rt::color::WHITE);

    // Array of the planes in the scene
    vector<plane> plane_set {pln0, pln1};


    /* *************************** */

    // Lights

    light light0(rt::vector(-500, 0, 600), rt::color::WHITE);
    light light1(rt::vector(0, 0, 1000),   my_red);
    light light2(rt::vector(750, 0, 900),  my_blue);

    // Array of the lights in the scene
    vector<light> light_set {light0, light1, light2};

    /* *************************** */

    // Screen
    int width = 960;
    int height = 680;
    double dist = 400; // Distance between the camera and the image

    // The camera is supposed to be on the origin of the space: (0,0,0)
    
    // Vector that will center the 'screen' in the scene
    rt::vector screen_center(width/2, height/2, 0);
    

    rt::screen scr(width,height);

    rt::color pixel_col;
    rt::vector direct(0, 0, 0);
    ray *r;
    // Progress bar
    printf("[..................................................]\r[");
    int pct = 0;
    int newpct = 0;

    for (int i = 0; i < width; i++) { // i is the abscissa
        for (int j = 0; j < height; j++) { //j is the ordinate

            direct = (rt::vector(i, j, dist)) - screen_center;
            r = new ray(rt::vector(0, 0, 0), direct, rt::color::WHITE);

            // pixel_col = launch_ray1(*r, sphere_set);
            // pixel_col = launch_ray2(*r, sphere_set, light_set);
            pixel_col = launch_ray3(*r, sphere_set, plane_set, light_set);

            scr.set_pixel(i, j, pixel_col);
        }

        // Progress bar
        newpct = 50*(i+1) / width;
        if (newpct > pct) {
            pct = newpct;
            printf("I");
        }
    }
    
    printf("\n");
    scr.update();

    while(not scr.wait_quit_event()) {}

    return EXIT_SUCCESS;
}
