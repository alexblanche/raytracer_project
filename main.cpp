#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <vector>
#include <queue>

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
    
    mutex m;
    // float cpt = 0;
    // float x = 100.0 / (((double) scene.width) * ((double) scene.height));

    PARALLEL_FOR_BEGIN(scene.width) {

        for (int j = 0; j < scene.height; j++) {
            
            const rt::vector& direct = (rt::vector(i, j, scene.distance) - scene.screen_center).unit();
            ray r = ray(scene.position, direct);
            const rt::color& pixel_col = //pathtrace_mult(r, scene, -1, 1, number_of_bounces);
                pathtrace(r, scene, number_of_bounces);
            
            const rt::color& current_color = matrix.at(i).at(j);
            const rt::color new_color = current_color + pixel_col;
            // cpt += 1;

            // Updating the color matrix
            m.lock();
            matrix.at(i).at(j) = new_color;
            m.unlock();
        }

        // m.lock();
        // printf("%f / 100\n", cpt * x);
        // m.unlock();
        
    } PARALLEL_FOR_END();
}


/* ********************************* */
/* ********************************* */

/* ********* MAIN FUNCTION ********* */


int main(int argc, char *argv[]) {

    /* Specification of the parameters through console arguments */
    unsigned int number_of_bounces = 2;
    if (argc > 1) {
        number_of_bounces = atoi(argv[1]);
    }

    printf("Number of bounces: %u\n", number_of_bounces);
    printf("Initialization...");

    /* *************************** */
    /* Scene description */
    
    scene scene("scene.txt");
    
    /* Orientation of the space:
    negative x on the left, positive on the right
    negative y at the top,  positive at the bottom (Be careful!!!)
    negative z behind the camera, positive in front of it
    */

    /* Creation of the triangles */
    const unsigned int triangles_per_terminal = 10;
    const unsigned int number_of_triangles = 10 * 4;
    //const unsigned int triangles_per_terminal = 100;
    //const unsigned int number_of_triangles = 100 * 16384;

    /* Time test:
        10 tpt, 5120 tr -> 34"
        80 tpt, 5120 tr -> 26"
        100 tpt, 1'638'400 tr -> estimated 78.5min = 4700" :'(
        tr * 320, time * 180... :(
        Not good considering the algorithm is supposed to be logarithmic...

        New optimization (method does_hit instead of measure_distance)
        
    */


    const double shift = (2 * 620) / (((double) number_of_triangles) - 1);

    const unsigned int nb_obj = object::set.size();

    for(unsigned int i = 0; i < number_of_triangles; i++) {
        new triangle(
            rt::vector(-620 + 0   + shift * ((double) i), -100, 600),
            rt::vector(-620 + 100 + shift * ((double) i),  100, 500),
            rt::vector(-620 + 80  + shift * ((double) i), -200, 700),
            light_material(rt::color(10, 180, 255), 0));
    }
    
    // Automatic bounding boxes definition
    queue<const bounding*> bounding_queue;
    
    // Creation of the terminal nodes and their non-terminal containers
    for(unsigned int i = 0; i < number_of_triangles / triangles_per_terminal; i++) {
        vector<unsigned int> v(triangles_per_terminal);
        for(unsigned int j = 0; j < triangles_per_terminal; j++) {
            v.at(j) = nb_obj + i * triangles_per_terminal + j;
        }
        bounding_queue.push(containing_objects(v));
    }

    // Grouping them by two until there is only one left
    while (bounding_queue.size() != 1) {
        const bounding* bd0 = bounding_queue.front();
        bounding_queue.pop();
        const bounding* bd1 = bounding_queue.front();
        bounding_queue.pop();

        const bounding* bd01 = containing_bounding(*bd0, *bd1);
        bounding_queue.push(bd01);
    }
    
    vector<unsigned int> indices(nb_obj);
    for (unsigned int i = 0; i < nb_obj; i++) {
        indices.at(i) = object::set.at(i)->get_index();
    }
    const bounding c(indices);
    const bounding* bd = bounding_queue.front();
    bounding::set = {bd, &c};
    
    /*
    // Temporary: pushing all objects to the bounding set
    vector<unsigned int> indices(object::set.size());
    for (unsigned int i = 0; i < object::set.size(); i++) {
        indices.at(i) = object::set.at(i)->get_index();
    }
    const bounding c(indices);
    bounding::set = {&c};
    */

    /* Test of usefulness of bounding boxes:
    5120 triangles, 5 bounces, 1 ray:
    with the object method: 6'10"
    with the bounding method:
    1 box:   3'50" (or slightly more)
    2 boxes: 2'37"
    4 boxes: 1'43"
    automatic boxing, 10 tr per box, total 1023 boxes: 46"
    -> optimization of the search (without obj_stack): 33"
    -> 80 tr per box, total 127 boxes:                 25"
    */

    /* ********************************************************** */

    /* Definition of the matrix in which we will write the image */
    vector<vector<rt::color>> matrix(scene.width, vector<rt::color>(scene.height));

    printf("\rInitialization complete, computing the first ray...");

    render_loop_parallel(matrix, scene, number_of_bounces);
    
    const rt::screen* scr = new rt::screen(scene.width, scene.height);
    scr->copy(matrix, scene.width, scene.height, 1);
    scr->update();

    printf("\r                                                   ");
    printf("\rNumber of rays per pixel: 1");

    scr->wait_quit_event();
    delete(scr);

    unsigned int number_of_rays = 1;
    bool stop = false;

    while (not stop) {
        number_of_rays ++;

        render_loop_parallel(matrix, scene, number_of_bounces);

        printf("\rNumber of rays per pixel: %u", number_of_rays);
        if (number_of_rays % 10 == 0) {
            const rt::screen* scr = new rt::screen(scene.width, scene.height);
            scr->copy(matrix, scene.width, scene.height, number_of_rays);
            scr->update();
            stop = scr->wait_quit_event();
            delete(scr);
        }
    }

    printf("\n");
    return EXIT_SUCCESS;
}