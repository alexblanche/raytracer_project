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
#include "src/scene/objects/headers/triangle.hpp"
#include "src/scene/objects/headers/box.hpp"

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
            
            const rt::vector& direct = (rt::vector(i, j, scene.distance) - scene.screen_center).unit();
            ray r = ray(scene.position, direct);
            const rt::color& pixel_col = //pathtrace_mult(r, scene, -1, 1, number_of_bounces);
                pathtrace(r, scene, number_of_bounces);
            
            const rt::color& current_color = matrix.at(i).at(j);
            const rt::color new_color (
                current_color.get_red() + pixel_col.get_red(),
                current_color.get_green() + pixel_col.get_green(),
                current_color.get_blue() + pixel_col.get_blue()
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

    /* 4 mirror spheres of decreasing specular_probability */
    const sphere sph0(rt::vector(-500, 0, 600), 120, material(rt::color::WHITE, rt::color(), 1, 0, 1.0, false));

    const sphere sph1(rt::vector(-166, 0, 600), 120, material(rt::color::WHITE, rt::color(), 1, 0, 0.6, false));
    const sphere sph2(rt::vector(166, 0, 600),  120, material(rt::color::WHITE, rt::color(), 1, 0, 0.1, false));
    const sphere sph3(rt::vector(500, 0, 600),  120, material(rt::color::WHITE, rt::color(), 1, 0, 0.05, false));

    // Triangle
    //const triangle tr0(rt::vector(-950, -500, 1150), rt::vector(950, -500, 50), rt::vector(-950, -500, 50), light_material(rt::color(10, 180, 255), 0));
    //const triangle tr1(rt::vector(-950, -500, 1150), rt::vector(950, -500, 1150), rt::vector(950, -500, 50), light_material(rt::color(10, 180, 255), 0));

    // Planes
    const plane pln0(0, -1, 0, rt::vector(0, 160, 0),   material(rt::color(10, 10, 10), rt::color(), 0.2, 0, 0.2, false));
    const plane pln1(0, 0, -1, rt::vector(0, 0, 1200),  light_material(rt::color::WHITE, 0));
    const plane pln2(1, 0, 0,  rt::vector(-1000, 0, 0), material(rt::color(255, 80, 80), 0));
    const plane pln3(-1, 0, 0, rt::vector(1000, 0, 0),  material(rt::color(80, 255, 80), 0));
    const plane pln4(0, 0, 1,  rt::vector(0, 0, 0),     light_material(rt::color(10, 180, 255), 0));/*light_material(rt::color::WHITE, 0));*/
    const plane pln5(0, 1, 0,  rt::vector(0, -600, 0),  material(rt::color(10, 10, 10), rt::color(), 0.8, 0, 0.5, false));//light_material(rt::color::WHITE, 1.5));
    
    const box bx0(rt::vector(0, -600, 600),
        rt::vector(1, 0, 0), rt::vector(0, 1, 0),
        800, 30, 400,
        light_material(rt::color::WHITE, 5));

    
    // Boxes
    
    /*
    const box bx0(rt::vector(166, -200, 600),
        rt::vector(100, 100, -100).unit(), rt::vector(-200, 100, -100).unit(),
        300, 200, 300,
        //light_material(rt::color(10, 180, 255), 3));
        material(rt::color(10, 180, 255), rt::color(), 1, 0, 0.3, false));
    */

    //const sphere sphl1(rt::vector(0, 0, 600), 30, obj_counter++, light_material(rt::color::WHITE, 30));

    /*const box bx0(rt::vector(100, -100, 600),
        rt::vector(1, 0, 0), rt::vector(0, 1, 0),
        500, 500, 500,
        //light_material(rt::color(10, 180, 255), 3));
        material(rt::color(10, 180, 255), rt::color(), 1, 0, 0.3, false));*/

    /*for(unsigned int i = 0; i < 1000; i++) {
        new triangle(rt::vector(-150 + 5*((double) i), -100, 700), rt::vector(-50 + 5*((double) i), 100, 700), rt::vector(-100 + 5*((double) i), -200, 700),
            light_material(rt::color(10+2*i, 180, 255), 0));
    }*/

    /* Test of usefulness of bounding boxes:
    
    For 30 rays, 5 bounces:
    1 box, no triangle:  47s
    0 box, 10 triangles: 54s
    0 box, 100 triangles: 180s (3min)
    0 box, 1000 triangles: estimated 30min
     */
    
    // Screen
    const int width = 1366;
    const int height = 768;
    const double dist = 400; // Distance between the camera and the image

    // The camera is supposed to be on the origin of the space: (0,0,0)
    
    // Vector that will center the 'screen' in the scene
    const rt::vector screen_center(width/2, height/2, 0);
    
    scene scene(object::set,
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
    printf("Initialization...");

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

    printf("\rNumber of rays per pixel: 1");

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