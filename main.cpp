#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <vector>

#include "src/screen/headers/color.hpp"
#include "src/scene/material/headers/material.hpp"
#include "src/screen/headers/screen.hpp"
#include "src/light/headers/ray.hpp"

#include "src/scene/objects/headers/object.hpp"
#include "src/scene/objects/headers/sphere.hpp"
#include "src/scene/objects/headers/plane.hpp"

#include "parallel/parallel.h"
#include "mingw.mutex.h"

#include "src/light/headers/hit.hpp"
#include "src/auxiliary/headers/randomgen.hpp"
#include "src/auxiliary/headers/tracing.hpp"
#include "src/scene/headers/scene.hpp"

using namespace std;

// Parallel for-loop macros
#define PARALLEL_FOR_BEGIN(nb_elements) parallel_for(nb_elements, [&](int start, int end){ for(int i = start; i < end; ++i)
#define PARALLEL_FOR_END()})


/* ********** Render loop ********** */

/* Parallel version */

void render_loop_parallel(vector<vector<rt::color>>& matrix, scene& scene, const unsigned int number_of_bounces) {
    
    std::mutex m;

    PARALLEL_FOR_BEGIN(scene.width) {

        for (int j = 0; j < scene.height; j++) {
            
            const rt::vector direct = (rt::vector(i, j, scene.distance) - scene.screen_center).unit();
            ray r = ray(scene.position, direct);
            const rt::color pixel_col = //pathtrace_mult(r, scene, -1, 1, number_of_bounces);
                pathtrace(r, scene, number_of_bounces);
            
            const rt::color current_color = matrix.at(i).at(j);
            const rt::color new_color (
                current_color.get_red() + pixel_col.get_red(), // min(pixel_col.get_red(), 255.0),
                current_color.get_green() + pixel_col.get_green(), // min(pixel_col.get_green(), 255.0),
                current_color.get_blue() + pixel_col.get_blue() // min(pixel_col.get_blue(), 255.0)
            );

            // Updating the color matrix
            m.lock();
            matrix.at(i).at(j) = new_color;
            m.unlock();
        }
        
    } PARALLEL_FOR_END();
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

    /* 4 metal spheres of increasing reflectivity */
    /*
    const sphere sph0(rt::vector(-500, 0, 600), 120, obj_counter++, material(rt::color::WHITE, 0.2));
    const sphere sph1(rt::vector(-166, 0, 600), 120, obj_counter++, material(rt::color::WHITE, 0.6));
    const sphere sph2(rt::vector(166, 0, 600),  120, obj_counter++, material(rt::color::WHITE, 0.95));
    const sphere sph3(rt::vector(500, 0, 600),  120, obj_counter++, material(rt::color::WHITE, 1));
    */

    /* 4 mirror spheres of decreasing specular_probability */
    const sphere sph0(rt::vector(-500, 0, 600), 120, obj_counter++, material(rt::color(255, 100, 100), rt::color(), 1, 0, 1.0, false));
    const sphere sph1(rt::vector(-166, 0, 600), 120, obj_counter++, material(rt::color(255, 100, 100), rt::color(), 1, 0, 0.6, false));
    const sphere sph2(rt::vector(166, 0, 600),  120, obj_counter++, material(rt::color(255, 100, 100), rt::color(), 1, 0, 0.1, false));
    const sphere sph3(rt::vector(500, 0, 600),  120, obj_counter++, material(rt::color(255, 100, 100), rt::color(), 1, 0, 0.05, false));

    // Planes
    const plane pln0(0, -1, 0, rt::vector(0, 160, 0),   obj_counter++, material(rt::color(10, 10, 10), rt::color(), 0.8, 0, 0.5, false));
    const plane pln1(0, 0, -1, rt::vector(0, 0, 1200),  obj_counter++, light_material(rt::color::WHITE, 0));
    const plane pln2(1, 0, 0,  rt::vector(-1000, 0, 0), obj_counter++, material(rt::color(255, 80, 80), 0));
    const plane pln3(-1, 0, 0, rt::vector(1000, 0, 0),  obj_counter++, material(rt::color(80, 255, 80), 0));
    const plane pln4(0, 0, 1,  rt::vector(0, 0, 0),     obj_counter++, /*light_material(rt::color::WHITE, 0));*/light_material(rt::color(10, 180, 255), 0));
    const plane pln5(0, 1, 0,  rt::vector(0, -600, 0),  obj_counter++, light_material(rt::color::WHITE, 2));
    
    // const sphere sphl1(rt::vector(0, 0, 600), 30, obj_counter++, light_material(rt::color::WHITE, 30));
    
    /* Object set */
    /* Storing pointers allow the overridden methods send and intersect (from sphere, plane)
       to be executed instead of the base (object) one */

    const vector<const object*> obj_set {&sph0, &sph1, &sph2, &sph3, &pln0, &pln1, &pln2, &pln3, &pln4, &pln5 /*, &sphl1*/};

    /*
    const sphere sph0(rt::vector(-200, 80, 600), 160, obj_counter++, material(rt::color(220, 220, 220), 1));
    const sphere sph1(rt::vector(200, 0, 600),  60, obj_counter++, light_material(rt::color::WHITE, 1));
    const plane pln0(0, -1, 0, rt::vector(0, 160, 0), obj_counter++, material(rt::color(220, 220, 220), 0));
    const vector<const object*> obj_set {&sph0, &sph1, &pln0};
    */
    

    // Screen
    const int width = 1366;
    const int height = 768;
    const double dist = 400; // Distance between the camera and the image

    // The camera is supposed to be on the origin of the space: (0,0,0)
    
    // Vector that will center the 'screen' in the scene
    const rt::vector screen_center(width/2, height/2, 0);
    
    scene scene(obj_set,
        rt::color(190, 235, 255), // Background color
        width, height, dist,
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

    render_loop_parallel(matrix, scene, number_of_bounces);
    
    scr.copy(matrix, width, height, 1);
    scr.update();

    unsigned int number_of_rays = 1;

    while (not scr.is_quit_event()) {
        number_of_rays ++;

        render_loop_parallel(matrix, scene, number_of_bounces);

        printf("\rNumber of rays per pixel: %u", number_of_rays);
        if (number_of_rays % 10 == 0) {
            scr.copy(matrix, width, height, number_of_rays);
            scr.update();
        }
    }

    printf("\n");
    return EXIT_SUCCESS;
}