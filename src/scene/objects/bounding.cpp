#include "scene/objects/box.hpp"
#include "scene/objects/triangle.hpp"
#include "scene/objects/bounding.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"

#include<limits>
numeric_limits<double> realbd;
const double infinity = realbd.infinity();

#include <stack>
#include <vector>

bounding::bounding()
    : is_terminal(true), b(NULL) {}

/* The vector n3 is taken as the cross product of n1 and n2 */
bounding::bounding(const bool is_terminal, const box* b, const std::vector<const object*>& content,
    const std::vector<const bounding*>& children)

    : is_terminal(is_terminal), b(b), content(content), children(children) {}

/* Container node constructor (only for first-level non-triangle objects) */
bounding::bounding(const std::vector<const object*>& content)

    : is_terminal(true), b(NULL), content(content) {}

/* Terminal node constructor (with a bounding box, containing triangles) */
bounding::bounding(const std::vector<const object*>& content, const box* b)

    : is_terminal(true), b(b), content(content) {}

/* Internal node constructor */
bounding::bounding(const box* b, const std::vector<const bounding*>& children)

    : is_terminal(false), b(b), children(children) {}



/* Tree-search version of the closest object to the ray r */

/* Auxiliary function to scene::find_closest_object_bounding :
   Places the children of the bounding on the bounding_stack if the box is hit,
   or determines the closest to the objects from the content if the bounding is terminal
   (if it is closest than the current closest_object, at a distance distance_to_closest,
   in which case the two variables are overwritten)
*/
void bounding::check_box(const ray& r,
    double& distance_to_closest, const object*& closest_object,
    std::stack<const bounding*>& bounding_stack) const {

    double d_cl = distance_to_closest;
    const object* cl_obj = closest_object;

    if (is_terminal) {
        if (b == NULL || b->is_hit_by(r)) {
            for (unsigned int i = 0; i < content.size(); i++) {
                const object* obj_i = content.at(i);
                const double d = obj_i->measure_distance(r);
                if (d < d_cl && d > 0.000001) {
                    d_cl = d;
                    cl_obj = obj_i;
                }
            }
        }
    }
    else {
        if (b->is_hit_by(r)) {
            for (unsigned int i = 0; i < children.size(); i++) {
                bounding_stack.push(children.at(i));
            }
        }
    }

    distance_to_closest = d_cl;
    closest_object = cl_obj;
}

/* Same as check_box, but the last child is stored in a pointer to avoid pushing and
   immediately popping on the stack */
void bounding::check_box_next(const ray& r,
    double& distance_to_closest, const object*& closest_object,
    std::stack<const bounding*>& bounding_stack,
    bool& bd_stored, const bounding*& next_bounding) const {

    if (is_terminal) {
        if (b == NULL || b->is_hit_by(r)) {
            for (unsigned int i = 0; i < content.size(); i++) {
                const object* obj_i = content.at(i);
                const double d = obj_i->measure_distance(r);
                if (d < distance_to_closest && d > 0.000001) {
                    distance_to_closest = d;
                    closest_object = obj_i;
                }
            }
        }
        bd_stored = false;
    }
    else {
        if (b->is_hit_by(r)) {
            const unsigned int last_index = children.size() - 1;
            for (unsigned int i = 0; i < last_index; i++) {
                bounding_stack.push(children.at(i));
            }
            // Last element of children
            next_bounding = children.at(last_index);
            bd_stored = true;
        }
        else {
            bd_stored = false;
        }
    }
}



/* Returns a non-terminal bounding box (standard, with n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1))
   containing the standard non-terminal bounding boxes bd0 and bd1 */
bounding* containing_bounding(const bounding& bd0, const bounding& bd1) {
    const box* const b0 = bd0.get_b();
    const box* const b1 = bd1.get_b();
    const double bd0l1 = b0->get_l1();
    const double bd0l2 = b0->get_l2();
    const double bd0l3 = b0->get_l3();
    const double bd1l1 = b1->get_l1();
    const double bd1l2 = b1->get_l2();
    const double bd1l3 = b1->get_l3();

    const rt::vector& pos0 = b0->get_position();
    const rt::vector& pos1 = b1->get_position();
    const rt::vector position(
        (pos1.x + bd1l1 + (pos0.x - bd0l1)) / 2,
        (pos1.y + bd1l2 + (pos0.y - bd0l2)) / 2,
        (pos1.z + bd1l3 + (pos0.z - bd0l3)) / 2);

    const box* b = new box(position,
        rt::vector(1,0,0), rt::vector(0,1,0),
        (pos1.x - pos0.x) + bd0l1 + bd1l1,
        (pos1.y - pos0.y) + bd0l2 + bd1l2,
        (pos1.z - pos0.z) + bd0l3 + bd1l3);
    
    return new bounding(b, {&bd0, &bd1});
}

/* Returns a non-terminal bounding box (standard, with n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1))
   containing the (finite) objects whose indices are in the obj vector */
bounding* containing_objects(const std::vector<const object*>& obj) {

    double min_x = infinity;
    double max_x = -infinity;
    double min_y = infinity;
    double max_y = -infinity;
    double min_z = infinity;
    double max_z = -infinity;

    /* Computation of the dimensions of the object set */
    for(unsigned int i = 0; i < obj.size(); i++) {
        double x_min, x_max, y_min, y_max, z_min, z_max;
        obj.at(i)->min_max_coord(x_min, x_max, y_min, y_max, z_min, z_max);

        if (x_max > max_x) {max_x = x_max;}
        if (x_min < min_x) {min_x = x_min;}
        if (y_max > max_y) {max_y = y_max;}
        if (y_min < min_y) {min_y = y_min;}
        if (z_max > max_z) {max_z = z_max;}
        if (z_min < min_z) {min_z = z_min;}
    }

    const box* b = new box(
        rt::vector((max_x + min_x) / 2, (max_y + min_y) / 2, (max_z + min_z) / 2),
        rt::vector(1, 0, 0), rt::vector(0, 1, 0),
        max_x - min_x,
        max_y - min_y,
        max_z - min_z
    );

    return new bounding(obj, b);
}




/****************************************************************************/

/*** Automatic bounding box generation tests (to be implemented later) ***/

/* Creation of the triangles */
    // const unsigned int triangles_per_terminal = 80;
    // const unsigned int number_of_triangles = 10 * 512;
    // const unsigned int triangles_per_terminal = 64;
    // const unsigned int number_of_triangles = 4096;
    // const unsigned int triangles_per_terminal = 6400;
    // const unsigned int number_of_triangles = 1638400;
    // const unsigned int number_of_triangles = 70000;

    /* Time test:
        100 tpt, 400 tr -> 7"
        100 tpt, 1600 tr -> 12"
        100 tpt, 3200 tr -> 18"
        100 tpt, 6400 tr -> 30"
        100 tpt, 12800 tr -> 55"
        100 tpt, 102400 tr -> 8'35"
        
        100 tpt,  1638400 tr -> 7142" = 119'
        1600 tpt, 1638400 tr -> 6833" = 114' (Optimized to 6645" = 110' with next_bounding pointer)
        6400 tpt, 1638400 tr -> 6687" = 111' (Optimized to 6557" = 109' with next_bounding pointer)
        no bounding, 1638400 tr -> 129251" = 35.9 hours

        Time (only) divided by 19.3... disappointing (optimized 19.7)

        4096 tr:
        no bounding -> 310" = 5'10"
        4 tpt       -> 34"
        16 tpt      -> 23"
        64 tpt      -> 21" (Optimized to 20" with next_bounding pointer)
        128 tpt     -> 22"
        512 tpt     -> 30"
        Improvement by a factor 14.76 (optimized 15.5)
    */

    /* Test: objects tested at each bounce (5 bounces) over all rays:
       64 tpt, 4096 tr:
       0: 150'052'721, 1: 204'548'204, 2: 132'922'393, 3: 145'187'026, 4: 149'899'062
       = (in objects per ray)
       0: 143, 1: 194, 2: 126, 3: 138, 4: 142 (average: 148.6 =~ 2.32 boxes, corresponding to an amplitude of 145 on the x-axis)
       Approximately as much time as 250 triangles (=~ 4096 / 16) with the object method

       6400 tpt, 1638400 tr:
       (at 2.8% of the first ray per pixel)
       0: 1054370820 (35558.2 per ray), 1: 1721877259 (58069.5 per ray), 2: 1222307253 (41221.7 per ray), 3: 1413194670 (47659.3 per ray), 4: 1497105051 (50489.2 per ray)
       (average = 46600 =~ 7.3 boxes, corresponding to an amplitude of 135 on the x-axis)
       
       Comparison with the object method:
       81920 triangles -> 6959" = 116'15"
       
       (new measures)
       81920 triangles -> 8204" = 136' (at 2.24%)
       80000 triangles -> 7973" = 132' (at 2.3%)
       70000 triangles -> 6631" = 110' (at 1.83%)
       (6400 tpt, 1638400 tr -> 6702" = 111' (at 2.68%))

       New optimization (is_hit_by specialized for standard boxes):
       6400 tpt, 1638400 tr -> 6674" = 111' (at 2.86%)
       Insignificant. (weird)
    */
    
    /*
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
    */
    
    /*
    // Temporary: pushing all objects to the bounding set
    vector<unsigned int> indices(object::set.size());
    for (unsigned int i = 0; i < object::set.size(); i++) {
        indices.at(i) = object::set.at(i)->get_index();
    }
    const bounding c(indices);
    bounding::set = {&c};
    */