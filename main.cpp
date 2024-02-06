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

#include "src/light/headers/hit.hpp"
#include "src/auxiliary/headers/randomgen.hpp"
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

    // Progress bar
    printf("[..................................................]\r[");
    int pct = 0;
    int newpct = 0;

    randomgen rg;

    const unsigned int number_of_rays = 30;
    const unsigned int number_of_bounces = 1;

    for (int i = 0; i < width; i++) { // i is the abscissa
        for (int j = 0; j < height; j = j + 2) { // j is the ordinate

            direct = rt::vector(i, j, dist) - screen_center;
            ray r = ray(rt::vector(0, 0, 0), direct.unit(), rt::color::WHITE);

            // pixel_col = raycast(*r, obj_set);
            // pixel_col = raytrace(*r, obj_set, light_set);

            pixel_col = pathtrace(r, obj_set, -1, number_of_rays, number_of_bounces, rg);

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
    const unsigned int number_of_rays, const unsigned int number_of_bounces,
    const rt::vector& screen_center, const vector<const object*>& obj_set) {
    
    std::mutex m;
    randomgen rg;

    printf("Number of rays at each bounce: %u, ", number_of_rays);
    printf("Number of bounces: %u\n", number_of_bounces);

    // Progress bar
    printf("[..................................................]\r[");
    int cpt = 0;
    int pct = 0;
    int newpct = 0;


    PARALLEL_FOR_BEGIN(width) {
        rt::vector direct;
        rt::color pixel_col;

        for (int j = 0; j < height; j++) {
            
            direct = rt::vector(i, j, dist) - screen_center;
            ray r = ray(rt::vector(0, 0, 0), direct.unit());

            // pixel_col = raycast(r, obj_set);
            // pixel_col = raytrace(r, obj_set, light_set);
            
            pixel_col = pathtrace(r, obj_set, -1, number_of_rays, number_of_bounces, rg);

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
    const sphere sphl1(rt::vector(166, 0, 600), 120, obj_counter++, material(rt::color::WHITE, rt::color(), 0.95, 0));
    const sphere sphl2(rt::vector(500, 0, 600), 120, obj_counter++, material(rt::color::WHITE, rt::color(), 1, 0));

    // Planes

    const plane pln0(0, -1, 0, rt::vector(0, 240, 0), obj_counter++, diffuse_material(rt::color(80, 80, 255)));
    const plane pln1(0, 0, -1, rt::vector(0, 0, 2000), obj_counter++, light_material(rt::color::WHITE, 1));
    const plane pln2(1, 0, 0, rt::vector(-1000, 0, 0), obj_counter++, diffuse_material(rt::color::RED));
    const plane pln3(1, 0, 0, rt::vector(1000, 0, 0), obj_counter++, diffuse_material(rt::color(80, 255, 80)));
    const plane pln4(0, 0, 1, rt::vector(0, 0, 0), obj_counter++, light_material(rt::color::WHITE, 0));
    const plane pln5(0, 1, 0, rt::vector(0, -600, 0), obj_counter++, light_material(rt::color::WHITE, 0));
    
    /* Object set */
    /* Storing pointers allow the overridden methods send and intersect (from sphere, plane)
       to be executed instead of the base (object) one */

    const vector<const object*> obj_set {&sph0, &sph1, &sphl1, &sphl2, &pln0, &pln1, &pln2, &pln3, &pln4, &pln5};

    

    /* *************************** */

    // Screen
    const int width = 1366;
    const int height = 768;
    const double dist = 500; // Distance between the camera and the image

    // The camera is supposed to be on the origin of the space: (0,0,0)
    
    // Vector that will center the 'screen' in the scene
    const rt::vector screen_center(width/2, height/2, 0);
    
    rt::screen scr(width, height);

    unsigned int number_of_rays = 5;
    unsigned int number_of_bounces = 2;
    if (argc > 1) {
        number_of_rays = atoi(argv[1]);
    }
    if (argc > 2) {
        number_of_bounces = atoi(argv[2]);
    }

    //render_loop_seq(scr, width, height, dist, screen_center, obj_set);
    render_loop_parallel(scr, width, height, dist, number_of_rays, number_of_bounces, screen_center, obj_set);

    
    scr.update();

    while(not scr.wait_quit_event()) {}

    return EXIT_SUCCESS;
    

    /**************************************************************************/
    /**************************************************************************/
    /* Debugging */
    
    /*

    const unsigned int number_of_rays = 30;
    const double twopi = 2 * 3.14159527;
    randomgen rg;

    int i = width/2;
    for (int j = 0; j < height; j ++) {
        const std::vector<double> rands01 = random_double_array(rg, number_of_rays, 1);
        const std::vector<double> rands0twopi = random_double_array(rg, number_of_rays, twopi);
        
        //int j = 4*height/5;
        rt::vector direct = rt::vector(i, j, dist) - screen_center;
        ray r = ray(rt::vector(0, 0, 0), direct.unit());
        //rt::color pixel_col = pathtrace(r, obj_set, number_of_rays, number_of_bounces);

        double d;
        double closest = infinity;
        unsigned int closest_index = 0;

        // Looking for the closest object
        
        for (unsigned int k = 0; k < obj_set.size(); k++) {

            d = obj_set.at(k)->measure_distance(r);

            if (d > 0.1 && d < closest) {
                closest = d;
                closest_index = k;
            }
        }

        printf("Closest_index = %u, distance = %f, ", closest_index, closest);

        const hit h = obj_set.at(closest_index)->compute_intersection(r, closest);
        const material m = obj_set.at(h.get_obj_index())->get_material();

        printf("Emission = %f, Color = (%u, %u, %u) \n", m.get_emission_intensity(), m.get_color().get_red(), m.get_color().get_green(), m.get_color().get_blue());

        rt::vector normal = h.get_normal();
        //printf("closest_Index: %u, normal: (%f, %f, %f)\n", h.get_obj_index(), normal.x, normal.y, normal.z);
        //rt::vector point = h.get_point();
        //printf("point: (%f, %f, %f) ", point.x, point.y, point.z);

        // Angle between the direction vector and the extremity of the disk (pi/2 * reflectivity)
        const double theta = 1.57079632679 * (1 - m.get_reflectivity());
        //printf("Theta: %f, distance to the disk: %f \n", theta, dist_disk);
        const std::vector<ray> bouncing_rays = h.random_reflect(number_of_rays, m.get_reflectivity(), theta);
        
        int see_light = 0;
        double total_height = 0;

        */

        //scr.set_pixel(15,15,rt::color::WHITE);
        //scr.draw_rect(10, height/2 - 10, width/2 - 10, 10, rt::color::RED);
        /*scr.clear();
        scr.draw_line(10, 10, width/2 - 10, 10, rt::color::WHITE);
        scr.draw_line(width/2 - 10, 10, width/2 - 10, height/2 - 10, rt::color::WHITE);
        scr.draw_line(10, 10, 10, height/2 - 10, rt::color::WHITE);
        scr.draw_line(10, height/2 - 10, width/2 - 10, height/2 - 10, rt::color::WHITE);
        */
        

        //scr.set_pixel(i, j, pathtrace(r, obj_set, 30, 1));
    //}

    /**************************************************************************/
    /**************************************************************************/


    // scr.set_pixel(5, 5, rt::color::WHITE);
    /*
    scr.update();

    while(not scr.wait_quit_event()) {}

    return EXIT_SUCCESS;
    */
}