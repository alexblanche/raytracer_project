#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <vector>
#include <ctime>

#include "src/screen/headers/color.hpp"
#include "src/scene/material/headers/material.hpp"
#include "src/screen/headers/screen.hpp"
#include "src/light/headers/ray.hpp"

#include "src/scene/sources/headers/source.hpp"
#include "src/scene/objects/headers/object.hpp"
#include "src/scene/objects/headers/sphere.hpp"
#include "src/scene/objects/headers/plane.hpp"

#include "parallel/parallel.h"
#include "mingw.mutex.h"

#include "src/auxiliary/headers/tracing.hpp"

using namespace std;

// Parallel for-loop macros
#define PARALLEL_FOR_BEGIN(nb_elements) parallel_for(nb_elements, [&](int start, int end){ for(int i = start; i < end; ++i)
#define PARALLEL_FOR_END()})


/* ********** Render loop ********** */

/* Sequential version */

void render_loop_seq(const rt::screen& scr, const int width, const int height, const double dist,
    const rt::vector& screen_center, const vector<const object*>& obj_set) { //, const vector<source>& light_set) {
    
    rt::color pixel_col;
    rt::vector direct(0, 0, 0);
    ray *r;

    // Progress bar
    printf("[..................................................]\r[");
    int pct = 0;
    int newpct = 0;

    const unsigned int number_of_rays = 10;
    const unsigned int number_of_bounces = 2;

    for (int i = 0; i < width; i++) { // i is the abscissa
        for (int j = 0; j < height; j++) { //j is the ordinate

            direct = rt::vector(i, j, dist) - screen_center;
            r = new ray(rt::vector(0, 0, 0), direct.unit(), rt::color::WHITE);

            // pixel_col = raycast(*r, obj_set);
            // pixel_col = raytrace(*r, obj_set, light_set);

            pixel_col = pathtrace(*r, obj_set, number_of_rays, number_of_bounces);

            scr.set_pixel(i, j, pixel_col);
        }

        // Progress bar
        newpct = 50 * (i+1) / width;
        if (newpct > pct) {
            pct = newpct;
            printf("I");
        }
    }
    
    printf("\n");
}


/* Parallel version */

void render_loop_parallel(const rt::screen& scr, const int width, const int height, const double dist,
    const rt::vector& screen_center, const vector<const object*>& obj_set) {
        //, const vector<source>& light_set) {
    
    std::mutex m;

    // Progress bar
    
    printf("[..................................................]\r[");
    int cpt = 0;
    int pct = 0;
    int newpct = 0;
    

    const unsigned int number_of_rays = 30;
    const unsigned int number_of_bounces = 2;

    PARALLEL_FOR_BEGIN(width) {
        ray *r;
        rt::vector direct;
        rt::color pixel_col;

        for (int j = 0; j < height; j++) {

            direct = rt::vector(i, j, dist) - screen_center;
            r = new ray(rt::vector(0, 0, 0), direct.unit(), rt::color::WHITE);

            // pixel_col = raycast(*r, obj_set);
            // pixel_col = raytrace(*r, obj_set, light_set);
            
            pixel_col = pathtrace(*r, obj_set, number_of_rays, number_of_bounces);

            m.lock();
            scr.set_pixel(i, j, pixel_col);
            m.unlock();
        }
        
        // Progress bar
        m.lock();
        cpt++;
        newpct = 50*(cpt+1) / width;
        if (newpct > pct) {
            pct = newpct;
            printf("I");
        }
        m.unlock();
        
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
    /* Scene description */
    
    unsigned int obj_counter = 0;

    /* Old raytracer scene */

    /*
    * Spheres and planes *
    const sphere sph0(rt::vector(-400, 0, 1000), 240, obj_counter++, material::MIRROR);
    const sphere sph1(rt::vector( 400, 0, 1000), 240, obj_counter++, diffuse_material(rt::color::WHITE));
    const plane pln0(0, -1, 0, rt::vector(0, 240, 0), obj_counter++, diffuse_material(rt::color::WHITE));
    const plane pln1(0, 0, -1, rt::vector(0, 0, 2000), obj_counter++, diffuse_material(rt::color::WHITE));
    const vector<const object*> obj_set {&sph0, &sph1, &pln0, &pln1};

    * Light sources *
    const source light0(rt::vector(-500, 0, 600), rt::color::WHITE);
    const source light1(rt::vector(0, 0, 1000),   my_red);
    const source light2(rt::vector(750, 0, 900),  my_blue);
    const vector<source> light_set {light0, light1, light2};
    */

    /* Path-tracer scene */

    
    // Spheres

    const sphere sph0(rt::vector(-400, 0, 1000), 200, obj_counter++, diffuse_material(rt::color::WHITE));
    const sphere sph1(rt::vector( 400, 0, 1000), 200, obj_counter++, diffuse_material(rt::color::WHITE));
    
    const sphere sphl1(rt::vector(   0, 0, 1000), 100, obj_counter++, light_material(rt::color::RED, 1));
    const sphere sphl2(rt::vector(-800, 0, 1000), 100, obj_counter++, light_material(rt::color::WHITE, 1));

    // Planes

    const plane pln0(0, 1, 0, rt::vector(0, 240, 0), obj_counter++, diffuse_material(rt::color::WHITE));
    const plane pln1(0, 0, 1, rt::vector(0, 0, 2000), obj_counter++, diffuse_material(rt::color::WHITE));
    
    /* Object set */
    /* Storing pointers allow the overridden methods send and intersect (from sphere, plane)
       to be executed instead of the base (object) one */

    const vector<const object*> obj_set {&sph0, &sph1, &pln0, &pln1, &sphl1, & sphl2};

    

    /* *************************** */

    // Screen
    const int width = 1366;
    const int height = 768;
    const double dist = 400; // Distance between the camera and the image

    // The camera is supposed to be on the origin of the space: (0,0,0)
    
    // Vector that will center the 'screen' in the scene
    const rt::vector screen_center(width/2, height/2, 0);
    
    const rt::screen scr(width, height);

    //render_loop_seq(scr, width, height, dist, screen_center, obj_set);
    render_loop_parallel(scr, width, height, dist, screen_center, obj_set);

    // scr.set_pixel(5, 5, rt::color::WHITE);
    
    scr.update();

    while(not scr.wait_quit_event()) {}

    return EXIT_SUCCESS;
}