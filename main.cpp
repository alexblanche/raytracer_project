#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <vector>
#include <ctime>

#include "src/screen/headers/color.hpp"
#include "src/scene/material/headers/material.hpp"
#include "src/screen/headers/screen.hpp"
#include "src/light/headers/ray.hpp"

//#include "src/scene/sources/headers/source.hpp"
#include "src/scene/objects/headers/object.hpp"
#include "src/scene/objects/headers/sphere.hpp"
#include "src/scene/objects/headers/plane.hpp"

#include "parallel/parallel.h"
#include "mingw.mutex.h"

#include "src/light/headers/hit.hpp"
#include "src/auxiliary/headers/randomgen.hpp"
#include "src/auxiliary/headers/tracing.hpp"
#include "src/scene/headers/scene.hpp"

#include "src/postprocess/headers/blur.hpp"

using namespace std;

// Parallel for-loop macros
#define PARALLEL_FOR_BEGIN(nb_elements) parallel_for(nb_elements, [&](int start, int end){ for(int i = start; i < end; ++i)
#define PARALLEL_FOR_END()})


/* ********** Render loop ********** */

/* Sequential version */

/*
void render_loop_seq(vector<vector<rt::color>>& matrix, scene& scene,
    const unsigned int number_of_rays, const unsigned int number_of_bounces) {

    // Progress bar
    printf("[..................................................]\r[");
    int pct = 0;
    int newpct = 0;

    for (int i = 0; i < scene.width; i++) { // i is the abscissa
        for (int j = 0; j < scene.height; j++) { // j is the ordinate

            const rt::vector direct = (rt::vector(i, j, scene.distance) - scene.screen_center).unit();
            const ray r = ray(scene.position, direct);
            const rt::color pixel_col = pathtrace_mult(r, scene, -1, number_of_rays, number_of_bounces);

            matrix.at(i).at(j) = pixel_col;
        }

        // Progress bar
        newpct = 50 * (i+1) / scene.width;
        if (newpct > pct) {
            pct = newpct;
            printf("I");
        }
        
    }
    
    printf("\n");
}
*/


/* Parallel version */

void render_loop_parallel(vector<vector<rt::color>>& matrix, scene& scene,
    const unsigned int number_of_rays, const unsigned int number_of_bounces) {
    
    std::mutex m;

    // Progress bar
    printf("[..................................................]\r[");
    int cpt = 0;
    int pct = 0;
    int newpct = 0;


    PARALLEL_FOR_BEGIN(scene.width) {

        for (int j = 0; j < scene.height; j++) {
            
            const rt::vector direct = (rt::vector(i, j, scene.distance) - scene.screen_center).unit();
            ray r = ray(scene.position, direct, rt::color::WHITE);
            const rt::color pixel_col = pathtrace(r, scene, -1, number_of_bounces);
            
            // Updating the color matrix
            const rt::color current_col = matrix.at(i).at(j);
            const rt::color new_col = rt::color(
                ((number_of_rays - 1) * current_col.get_red()   + pixel_col.get_red())   / number_of_rays,
                ((number_of_rays - 1) * current_col.get_green() + pixel_col.get_green()) / number_of_rays,
                ((number_of_rays - 1) * current_col.get_blue()  + pixel_col.get_blue())  / number_of_rays);

            m.lock();
            matrix.at(i).at(j) = new_col;
            m.unlock();
        }
        
        // Progress bar
        m.lock();
        cpt++;
        newpct = 50*(cpt+1) / scene.width;
        if (newpct > pct) {
            pct = newpct;
            printf("I");
        }
        m.unlock();
        
    } PARALLEL_FOR_END();

    printf("\n");
}




/* ********************************* */
/* ********************************* */

/* ********* MAIN FUNCTION ********* */


int main(int argc, char *argv[]) {

    /* Orientation of the space:
    negative x on the left, positive on the right
    negative y at the top,  positive at the bottom (Be careful!!!)
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

    /*

    const sphere sph0(rt::vector(-400, 0, 1000), 240, obj_counter++, material(rt::color::WHITE, rt::color(), 0.98, 0)); //material(rt::color(255, 40, 40), rt::color(), 0.98, 0));
    const sphere sph1(rt::vector( 400, 0, 1000), 240, obj_counter++, diffuse_material(rt::color::WHITE));
    
    const sphere sphl1(rt::vector( 500, 140, 660), 100, obj_counter++, light_material(rt::color::WHITE, 1));
    const sphere sphl2(rt::vector(-800, 0, 800), 100, obj_counter++, light_material(rt::color::WHITE, 1));

    */

    const sphere sph0(rt::vector(-500, 0, 600), 120, obj_counter++, material(rt::color::WHITE, rt::color(), 0.2, 0));
    const sphere sph1(rt::vector(-166, 0, 600), 120, obj_counter++, material(rt::color::WHITE, rt::color(), 0.6, 0));
    const sphere sph2(rt::vector(166, 0, 600), 120, obj_counter++, material(rt::color::WHITE, rt::color(), 0.95, 0));
    const sphere sph3(rt::vector(500, 0, 600), 120, obj_counter++, material(rt::color::WHITE, rt::color(), 1, 0));

    // Planes

    const plane pln0(0, -1, 0, rt::vector(0, 160, 0),   obj_counter++, material(rt::color(80, 80, 255), rt::color(), 0, 0));
    const plane pln1(0, 0, -1, rt::vector(0, 0, 2000),  obj_counter++, light_material(rt::color::WHITE, 1));
    const plane pln2(1, 0, 0,  rt::vector(-1000, 0, 0), obj_counter++, material(rt::color::RED, rt::color(), 0, 0));
    const plane pln3(1, 0, 0,  rt::vector(1000, 0, 0),  obj_counter++, material(rt::color(80, 255, 80), rt::color(), 0, 0));
    const plane pln4(0, 0, 1,  rt::vector(0, 0, 0),     obj_counter++, light_material(rt::color::WHITE, 1));
    const plane pln5(0, 1, 0,  rt::vector(0, -600, 0),  obj_counter++, material(rt::color::WHITE, rt::color(), 0, 0));

    const sphere sphl1(rt::vector(0, 0, 600), 30, obj_counter++, light_material(rt::color::WHITE, 1));
    
    /* Object set */
    /* Storing pointers allow the overridden methods send and intersect (from sphere, plane)
       to be executed instead of the base (object) one */

    const vector<const object*> obj_set {&sph0, &sph1, &sph2, &sph3, &pln0, &pln1, &pln2, &pln3, &pln4, &pln5, &sphl1};

    // Screen
    const int width = 1366;
    const int height = 768;
    const double dist = 500; // Distance between the camera and the image

    // The camera is supposed to be on the origin of the space: (0,0,0)
    
    // Vector that will center the 'screen' in the scene
    const rt::vector screen_center(width/2, height/2, 0);
    
    scene scene(obj_set,
        rt::color(255, 255, 255), // Background color
        width,
        height,
        dist,
        rt::vector(0, 0, 0), // Camera position
        screen_center);

    /* ********************************************************** */

    /* Specification of the parameters through console arguments */
    //unsigned int number_of_rays = 5;
    unsigned int number_of_bounces = 2;
    if (argc > 1) {
        number_of_bounces = atoi(argv[1]);
    }
    /*
    if (argc > 2) {
        number_of_bounces = atoi(argv[2]);
    }
    */

    //printf("Number of rays at each bounce: %u, ", number_of_rays);
    printf("Number of bounces: %u\n", number_of_bounces);

    /* Definition of the matrix in which we will write the image */
    vector<vector<rt::color>> matrix(width, vector<rt::color>(height));

    //render_loop_seq(matrix, scene, number_of_rays, number_of_bounces);
    //render_loop_parallel(matrix, scene, number_of_rays, number_of_bounces);

    // BMP file generation
    //generate_bmp(matrix, "pathtrace.bmp");

    // Display of the image on screen
    const rt::screen scr(width, height);

    render_loop_parallel(matrix, scene, 1, number_of_bounces);
    
    scr.copy(matrix, width, height);
    scr.update();

    unsigned int number_of_rays = 1;

    while (not scr.is_quit_event()) {
        number_of_rays ++;
        printf("Number of rays per pixel: %u\n", number_of_rays);

        render_loop_parallel(matrix, scene, number_of_rays, number_of_bounces);
    
        scr.copy(matrix, width, height);
        scr.update();
    }

    return EXIT_SUCCESS;
}