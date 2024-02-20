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

#include "src/scene/objects/headers/bounding.hpp"

using namespace std;

// Parallel for-loop macros
#define PARALLEL_FOR_BEGIN(nb_elements) parallel_for(nb_elements, [&](int start, int end){ for(int i = start; i < end; ++i)
#define PARALLEL_FOR_END()})


/* ********** Render loop ********** */

void render_loop_parallel(vector<vector<rt::color>>& matrix, scene& scene, const unsigned int number_of_bounces) {
    
    std::mutex m;

    PARALLEL_FOR_BEGIN(scene.width) {

        for (int j = 0; j < scene.height; j++) {
            
            const rt::vector& direct = (rt::vector(i, j, scene.distance) - scene.screen_center).unit();
            ray r = ray(scene.position, direct);
            const rt::color& pixel_col = //pathtrace_mult(r, scene, -1, 1, number_of_bounces);
                pathtrace(r, scene, number_of_bounces);
            
            const rt::color& current_color = matrix.at(i).at(j);
            // const rt::color new_color (
            //     current_color.get_red() + pixel_col.get_red(),
            //     current_color.get_green() + pixel_col.get_green(),
            //     current_color.get_blue() + pixel_col.get_blue()
            // );
            const rt::color new_color = current_color + pixel_col;

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

    /* Orientation of the space:
    negative x on the left, positive on the right
    negative y at the top,  positive at the bottom (Be careful!!!)
    negative z behind the camera, positive in front of it
    */

    /* *************************** */
    /* Scene description */

    /* 4 mirror spheres of decreasing specular_probability */
    // const sphere sph0(rt::vector(-500, 0, 600), 120, material(rt::color::WHITE, rt::color(), 1, 0, 1.0, false));

    // const sphere sph1(rt::vector(-166, 0, 600), 120, material(rt::color::WHITE, rt::color(), 1, 0, 0.6, false));
    // const sphere sph2(rt::vector(166, 0, 600),  120, material(rt::color::WHITE, rt::color(), 1, 0, 0.1, false));
    // const sphere sph3(rt::vector(500, 0, 600),  120, material(rt::color::WHITE, rt::color(), 1, 0, 0.05, false));

    // Triangle
    //const triangle tr0(rt::vector(-950, -500, 1150), rt::vector(950, -500, 50), rt::vector(-950, -500, 50), light_material(rt::color(10, 180, 255), 0));
    //const triangle tr1(rt::vector(-950, -500, 1150), rt::vector(950, -500, 1150), rt::vector(950, -500, 50), light_material(rt::color(10, 180, 255), 0));

    // Planes
    const plane pln0(0, -1, 0, rt::vector(0, 160, 0),   material(rt::color(10, 10, 10), rt::color(), 0.2, 0, 0.2, false));
    const plane pln1(0, 0, -1, rt::vector(0, 0, 1200),  light_material(rt::color::WHITE, 0));
    const plane pln2(1, 0, 0,  rt::vector(-1000, 0, 0), material(rt::color(255, 80, 80), 0));
    const plane pln3(-1, 0, 0, rt::vector(1000, 0, 0),  material(rt::color(80, 255, 80), 0));
    const plane pln4(0, 0, 1,  rt::vector(0, 0, 0),     light_material(rt::color(10, 180, 255), 0)); /*light_material(rt::color::WHITE, 0));*/
    const plane pln5(0, 1, 0,  rt::vector(0, -600, 0),  material(rt::color(10, 10, 10), rt::color(), 0.8, 0, 0.5, false)); //light_material(rt::color::WHITE, 1.5));
    
    const box bx_light(rt::vector(0, -600, 600),
        rt::vector(1, 0, 0), rt::vector(0, 1, 0),
        800, 30, 400,
        light_material(rt::color::WHITE, 5));

    
    // Bounding boxes
    /*
    // Content 1 (terminal): contains the two balls on the left
    vector<unsigned int> obv1 = {sph0.get_index(), sph1.get_index()};
    const bounding c1(obv1);

    // Box 1: containing Content 1
    const box bx1((0.5 * sph0.get_position() + 0.5 * sph1.get_position()),
        rt::vector(1, 0, 0), rt::vector(0, 1, 0),
        sph1.get_position().x + sph1.get_radius() - (sph0.get_position().x - sph0.get_radius()),
        sph1.get_position().y + sph1.get_radius() - (sph0.get_position().y - sph0.get_radius()),
        sph1.get_position().z + sph1.get_radius() - (sph0.get_position().z - sph0.get_radius()));
    vector<const bounding*> bdv1 = {&c1};
    const bounding bd1(&bx1, bdv1);

    // Box 2 (terminal): contains the two balls on the right
    vector<unsigned int> obv2 = {sph2.get_index(), sph3.get_index()};
    const bounding c2(obv2);

    // Box 2: containing Content 2
    const box bx2((0.5 * sph2.get_position() + 0.5 * sph3.get_position()),
        rt::vector(1, 0, 0), rt::vector(0, 1, 0),
        sph3.get_position().x + sph3.get_radius() - (sph2.get_position().x - sph2.get_radius()),
        sph3.get_position().y + sph3.get_radius() - (sph2.get_position().y - sph2.get_radius()),
        sph3.get_position().z + sph3.get_radius() - (sph2.get_position().z - sph2.get_radius()));
    vector<const bounding*> bdv2 = {&c2};
    const bounding bd2(&bx2, bdv2);

    // Box 3: contains boxes 1 and 2
    
    const box bx3((0.5 * bx1.get_position() + 0.5 * bx2.get_position()),
        rt::vector(1, 0, 0), rt::vector(0, 1, 0),
        sph3.get_position().x + sph3.get_radius() - (sph0.get_position().x - sph0.get_radius()),
        sph3.get_position().y + sph3.get_radius() - (sph0.get_position().y - sph0.get_radius()),
        sph3.get_position().z + sph3.get_radius() - (sph0.get_position().z - sph0.get_radius()));
    vector<const bounding*> bdv3 = {&bd1, &bd2};
    const bounding bd3(&bx3, bdv3);
    

    // Box 4: contains bx_light and box 3

    vector<unsigned int> obv4 = {bx_light.get_index()};
    const bounding c4(obv4);
    
    const box bx4((0.5 * bx3.get_position() + 0.5 * bx_light.get_position() + rt::vector(0, 0.5 * (120 - 15), 0)),
        rt::vector(1, 0, 0), rt::vector(0, 1, 0),
        sph3.get_position().x + sph3.get_radius() - (sph0.get_position().x - sph0.get_radius()),
        sph3.get_position().y + sph3.get_radius() - (-615),//(-615),
        400);
        //(sph3.get_position().z + sph3.get_radius() - (sph0.get_position().z - sph0.get_radius())));

    vector<const bounding*> bdv4 = {&bd3, &c4};
    const bounding bd4(&bx4, bdv4);

    // Content 5 (terminal): contains the six planes
    vector<unsigned int> obv5 = {pln0.get_index(), pln1.get_index(), pln2.get_index(), pln3.get_index(), pln4.get_index(), pln5.get_index()};//, bx0.get_index()};
    const bounding c5(obv5);

    // Bounding box set: contains boxes 4 and 5
    bounding::set = {&bd4, &c5};
    
    */

    /*const box bx0(rt::vector(166, -200, 600),
        rt::vector(100, 100, -100).unit(), rt::vector(-200, 100, -100).unit(),
        300, 200, 300,
        //light_material(rt::color(10, 180, 255), 3));
        material(rt::color(10, 180, 255), rt::color(), 1, 0, 0.3, false));*/
    
    //const sphere sphl1(rt::vector(0, 0, 600), 30, obj_counter++, light_material(rt::color::WHITE, 30));

    /*const box bx0(rt::vector(100, -100, 600),
        rt::vector(1, 0, 0), rt::vector(0, 1, 0),
        500, 500, 500,
        //light_material(rt::color(10, 180, 255), 3));
        material(rt::color(10, 180, 255), rt::color(), 1, 0, 0.3, false));*/

    const unsigned int number_of_triangles = 10*512;
    const double shift = (2 * 620) / (((double) number_of_triangles) - 1);

    for(unsigned int i = 0; i < number_of_triangles; i++) {
        new triangle(
            rt::vector(-620 + 0   + shift * ((double) i), -100, 600),
            rt::vector(-620 + 100 + shift * ((double) i),  100, 500),
            rt::vector(-620 + 80  + shift * ((double) i), -200, 700),
            light_material(rt::color(10, 180, 255), 0));
    }
    
    
    vector<unsigned int> obvtr0(number_of_triangles/4);
    for(unsigned int i = 0; i < number_of_triangles/4; i++) {
        obvtr0.at(i) = 7 + i;
    }
    const bounding ctr0(obvtr0);

    
    vector<unsigned int> obvtr1(number_of_triangles/4);
    for(unsigned int i = 0; i < number_of_triangles/4; i++) {
        obvtr1.at(i) = 7 + (number_of_triangles/4) + i;
    }
    const bounding ctr1(obvtr1);

    vector<unsigned int> obvtr2(number_of_triangles/4);
    for(unsigned int i = 0; i < number_of_triangles/4; i++) {
        obvtr2.at(i) = 7 + (number_of_triangles/2) + i;
    }
    const bounding ctr2(obvtr2);

    vector<unsigned int> obvtr3(number_of_triangles/4);
    for(unsigned int i = 0; i < number_of_triangles/4; i++) {
        obvtr3.at(i) = 7 + (3*number_of_triangles/4) + i;
    }
    const bounding ctr3(obvtr3);

    const box b0(rt::vector((-310+100+(-620))/2, -50, 600),
        rt::vector(1,0,0), rt::vector(0,1,0),
        (-310+100 - (-620)), 302, 202);
    const box b1(rt::vector((-310+100)/2, -50, 600),
        rt::vector(1,0,0), rt::vector(0,1,0),
        100-(-310), 302, 202);
    const box b2(rt::vector((310+100)/2, -50, 600),
        rt::vector(1,0,0), rt::vector(0,1,0),
        310+100, 302, 202);
    const box b3(rt::vector((620+100 + 310)/2, -50, 600),
        rt::vector(1,0,0), rt::vector(0,1,0),
        620+100-310, 302, 202);

    const bounding bd0(&b0, {&ctr0});
    const bounding bd1(&b1, {&ctr1});
    const bounding bd2(&b2, {&ctr2});
    const bounding bd3(&b3, {&ctr3});

    const box b01 = containing(bd0, bd1);
    const bounding bd01 = bounding(&b01, {&bd0, &bd1});
    const box b23 = containing(bd2, bd3);
    const bounding bd23 = bounding(&b23, {&bd2, &bd3});

    const box b = containing(bd01, bd23);
    const bounding bd = bounding(&b, {&bd01, &bd23});

    // Content (terminal): contains the six planes and the light box
    vector<unsigned int> obv = {pln0.get_index(), pln1.get_index(), pln2.get_index(), pln3.get_index(), pln4.get_index(), pln5.get_index(), bx_light.get_index()};
    const bounding c(obv);

    bounding::set = {&bd, &c};
    

    /* Test of usefulness of bounding boxes:
    5120 triangles, 5 bounces, 1 ray:
    with the object method:   6'10"
    with the bounding method:
    1 box:   3'50" (or slightly more)
    2 boxes: 2'37"
    4 boxes: 1'43"
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

    /* Definition of the matrix in which we will write the image */
    vector<vector<rt::color>> matrix(width, vector<rt::color>(height));

    //render_loop_seq(matrix, scene, number_of_rays, number_of_bounces);
    //render_loop_parallel(matrix, scene, number_of_rays, number_of_bounces);

    // BMP file generation
    //generate_bmp(matrix, "pathtrace.bmp");

    // Display of the image on screen
    //const rt::screen scr(width, height);

    printf("\rInitialization complete, computing the first ray...");

    render_loop_parallel(matrix, scene, number_of_bounces);
    
    const rt::screen scr(width, height);
    scr.copy(matrix, width, height, 1);
    scr.update();
    scr.wait_quit_event();
    delete(&scr);

    printf("\r                                                   ");
    printf("\rNumber of rays per pixel: 1");

    unsigned int number_of_rays = 1;

    while (not scr.is_quit_event()) {
        number_of_rays ++;

        render_loop_parallel(matrix, scene, number_of_bounces);

        printf("\rNumber of rays per pixel: %u", number_of_rays);
        if (number_of_rays % 10 == 0) {
            const rt::screen scr(width, height);
            scr.copy(matrix, width, height, number_of_rays);
            scr.update();
            scr.wait_quit_event();
            delete(&scr);
        }
    }

    printf("\n");
    return EXIT_SUCCESS;
}