/*** Legacy raytracer (2014) ***/

#include <iostream>
#include <vector>

#include "screen/screen.hpp"

#include "legacy/source.hpp"
#include "legacy/objects/sphere.hpp"
#include "legacy/objects/plane.hpp"

#include "legacy/raytracing/tracing.hpp"

#include "parallel/parallel.h"
#ifdef __unix__
#include <mutex>
#else
#include "mingw.mutex.h"
#endif

#include <chrono>

// Parallel for-loop macros
#define PARALLEL_FOR_BEGIN(nb_elements) parallel_for(nb_elements, [&](int start, int end){ for(int i = start; i < end; i++)
#define PARALLEL_FOR_END()})


/* ********** Render loop ********** */

/* Sequential version */

void render_loop_seq(const rt::screen& scr, const int width, const int height, const double& dist,
    const rt::vector& screen_center, const std::vector<const object*>& obj_set, const std::vector<source>& light_set) {

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {

            const rt::vector direct = rt::vector(i, j, dist) - screen_center;
            ray r = ray(rt::vector(0, 0, 0), direct.unit());

            rt::color pixel_col = raytrace(r, obj_set, light_set);

            scr.set_pixel(i, j, pixel_col);
        }
    }
}


/* Parallel version */

void render_loop_parallel(const rt::screen& scr, const int width, const int height, const double& dist,
    const rt::vector& screen_center, const std::vector<const object*>& obj_set, const std::vector<source>& light_set) {
    
    std::mutex m;

    PARALLEL_FOR_BEGIN(width) {

        rt::color output[height];

        for (int j = 0; j < height; j++) {

            const rt::vector direct = rt::vector(i, j, dist) - screen_center;
            ray r = ray(rt::vector(0, 0, 0), direct.unit());

            //rt::color pixel_col = raytrace(r, obj_set, light_set);
            output[j] = raytrace(r, obj_set, light_set);

            // m.lock();
            // scr.set_pixel(i, j, pixel_col);
            // m.unlock();
        }

        m.lock();
        for(int j = 0; j < height; j++) {
            scr.set_pixel(i, j, output[j]);
        }
        m.unlock();

    } PARALLEL_FOR_END();
}

long int get_time () {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
}
        

/* ********************************* */
/* ********************************* */

/* ********* MAIN FUNCTION ********* */


int main(int /*argc*/, char **/*argv*/) {

    const rt::color my_red(230, 15, 15);
    const rt::color my_green(15, 230, 15);
    const rt::color my_blue(15, 15, 230);
    const rt::color my_white(230, 230, 230);
    const rt::color my_black(15, 15, 15);
    /* Non-pure colors, to prevent the "black hole" effect when
     a red surface is under a blue spot */

    /* Orientation of the space:
    negative x on the left, positive on the right
    negative y on the top,  positive on the bottom
    negative z behind the camera, positive in front of it
    */

    /* *************************** */
    /* Scene description */

    // Spheres

    // Sphere 00
    const sphere sph0(rt::vector(-400,0,1000), 240, rt::color::WHITE);
    // Sphere 1
    const sphere sph1(rt::vector( 400,0,1000), 240, rt::color::WHITE);

    // Planes

    // Plane 0
    const plane pln0(0, -1, 0, rt::vector(0, 240, 0), rt::color::WHITE);
    // Plane 1
    const plane pln1(0, 0, -1, rt::vector(0, 0, 2000), rt::color::WHITE);

    /* Object set */
    /* Storing pointers allow the overridden methods send and intersect (from sphere, plane)
       to be executed instead of the base (object) one */
    const vector<const object*> obj_set {&sph0, &sph1, &pln0, &pln1};


    /* *************************** */

    // Light sources

    const source light0(rt::vector(-500, 0, 600), rt::color::WHITE);
    const source light1(rt::vector(0, 0, 1000),   my_red);
    const source light2(rt::vector(750, 0, 900),  my_blue);

    // Array of the lights in the scene
    const vector<source> light_set {light0, light1, light2};

    /* *************************** */

    // Screen
    const int width = 1920;
    const int height = 1080;
    const double dist = 400; // Distance between the camera and the image

    // The camera is supposed to be on the origin of the space: (0,0,0)
    
    // Vector that will center the 'screen' in the scene
    const rt::vector screen_center(width/2, height/2, 0);
    
    const rt::screen scr(width, height);

    const long int time_init = get_time();

    render_loop_seq(scr, width, height, dist, screen_center, obj_set, light_set);
    //render_loop_parallel(scr, width, height, dist, screen_center, obj_set, light_set);

    const long int curr_time = get_time();
    
    printf("%ld ms\n", curr_time - time_init);
    fflush(stdout);
    
    scr.update();

    while(not scr.wait_quit_event()) {}

    return EXIT_SUCCESS;
}