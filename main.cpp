#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <vector>

#include "src/screen/headers/color.hpp"
#include "src/screen/headers/screen.hpp"

/*
#include "src/light/headers/vector.hpp"
#include "src/light/headers/hit.hpp"
*/

#include "src/light/headers/source.hpp"
#include "src/light/headers/ray.hpp"

/*
#include "src/objects/headers/object.hpp"
*/
#include "src/objects/headers/sphere.hpp"
#include "src/objects/headers/plane.hpp"


#include "parallel/parallel.h"
#include "mingw.mutex.h"

using namespace std;

// Parallel for-loop macros
#define PARALLEL_FOR_BEGIN(nb_elements) parallel_for(nb_elements, [&](int start, int end){ for(int i = start; i < end; ++i)
#define PARALLEL_FOR_END()})



/* ************************************************************************ */

/* Light application */

/* Returns a vector of colors resulting from the application of each light source on the given hit,
    while NOT taking other objects into account. */
/* The formula for the addition of lights is:
    (r1,g1,b1) + (r2,g2,b2) = (min(r1+r2, 255), min(g1+g2, 255), min(b1+b2, 255)) */
vector<rt::color> apply_lights(const hit& h, const vector<source>& light_set) {

    const unsigned int n = light_set.size();
    vector<rt::color> color_set(n);

    for (unsigned int i = 0 ; i < n ; i++) {
        color_set.at(i) = light_set.at(i).apply(h);
    }

    return color_set;
}

/* Returns a vector of colors resulting from the application of each light source on the given hit,
    while taking other objects into account. */
vector<rt::color> apply_lights_obj(const hit& h, const vector<object>& obj_set, const vector<source>& light_set) {

    const unsigned int n = light_set.size();
    vector<rt::color> color_set(n);

    for (unsigned int i = 0 ; i < n ; i++) {
        color_set.at(i) = light_set.at(i).apply_obj(h, obj_set);
    }

    return color_set;
}



/* ********************************* */
/* ********** Render loop ********** */

/* Sequential version */

void render_loop_seq(const rt::screen& scr, const int width, const int height, const double dist,
    const rt::vector& screen_center, const vector<object>& obj_set, const vector<source>& light_set) {
    
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

            pixel_col = r->cast_ray(obj_set);
            // pixel_col = launch_ray(*r, obj_set, light_set);

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
}

/* Parallel version */

void render_loop_parallel(const rt::screen& scr, const int width, const int height, const double dist,
    const rt::vector& screen_center, const vector<object>& obj_set, const vector<source>& light_set) {
    
    std::mutex m;

    // Progress bar
    /*
    printf("[..................................................]\r[");
    int cpt = 0;
    int pct = 0;
    int newpct = 0;
    */

    PARALLEL_FOR_BEGIN(width) {
        ray *r;
        rt::vector direct;
        rt::color pixel_col;

        for (int j = 0; j < height; j++) {

            direct = (rt::vector(i, j, dist)) - screen_center;
            r = new ray(rt::vector(0, 0, 0), direct, rt::color::WHITE);

            // pixel_col = cast_ray(*r, obj_set);
            pixel_col = launch_ray(*r, obj_set, light_set);

            m.lock();
            scr.set_pixel(i, j, pixel_col);
            m.unlock();
        }
        
        // Progress bar
        /*m.lock();
        cpt++;
        newpct = 50*(cpt+1) / width;
        if (newpct > pct) {
            pct = newpct;
            printf("I");
        }
        m.unlock();
        */
    } PARALLEL_FOR_END();
}




/* ********************************* */
/* ********************************* */

/* ********* MAIN FUNCTION ********* */


int main(int argc, char *argv[]) {

    const rt::color my_red(230, 15, 15);
    const rt::color my_green(15, 230, 15);
    const rt::color my_blue(15, 15, 230);
    const rt::color my_white(230, 230, 230);
    const rt::color my_black(15, 15, 15);
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
    const sphere sph0(rt::vector(-400,0,1000), 240, rt::color::WHITE);
    // Sphere 1
    const sphere sph1(rt::vector( 400,0,1000), 240, rt::color::WHITE);

    // Array of the spheres in the scene
    //vector<sphere> sphere_set {sph0, sph1};

    /* *************************** */

    // Planes

    // Plane 0
    const plane pln0(0, 1, 0, rt::vector(0, 240, 0), my_red);//rt::color::WHITE);
    // Plane 1
    const plane pln1(0, 0, 1, rt::vector(0, 0, 2000), my_blue); //rt::color::WHITE);

    // Array of the planes in the scene
    //vector<plane> plane_set {pln0, pln1};

    const vector<object> obj_set {sph0, sph1, pln0, pln1};


    /* *************************** */

    // Light sources

    const source light0(rt::vector(-500, 0, 600), rt::color::WHITE);
    const source light1(rt::vector(0, 0, 1000),   my_red);
    const source light2(rt::vector(750, 0, 900),  my_blue);

    // Array of the lights in the scene
    const vector<source> light_set {light0, light1, light2};

    /* *************************** */

    // Screen
    const int width = 1366;
    const int height = 768;
    const double dist = 400; // Distance between the camera and the image

    // The camera is supposed to be on the origin of the space: (0,0,0)
    
    // Vector that will center the 'screen' in the scene
    const rt::vector screen_center(width/2, height/2, 0);
    
    const rt::screen scr(width,height);

    render_loop_seq(scr, width, height, dist, screen_center, obj_set, light_set);
    //render_loop_parallel(scr, width, height, dist, screen_center, obj_set, light_set);
    
    scr.update();

    while(not scr.wait_quit_event()) {}

    return EXIT_SUCCESS;
}

/* Time for 1366x768 (in s):
Seq:        8.12    8.37    8.35
Parallel:   2.17    2.17    2.37
Result: 3.7 times faster!
*/