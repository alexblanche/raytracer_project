#include "headers/box.hpp"
#include "headers/triangle.hpp"
#include "headers/bounding.hpp"

#include "../../light/headers/vector.hpp"
#include "../../light/headers/hit.hpp"
#include "../material/headers/material.hpp"

#include<limits>
numeric_limits<double> realbd;
const double infinity = realbd.infinity();

#include <stack>
#include <vector>

std::vector<const bounding*> bounding::set;

bounding::bounding()
    : is_terminal(true) {}

/* The vector n3 is taken as the cross product of n1 and n2 */
bounding::bounding(const bool is_terminal, const box* b, const std::vector<unsigned int>& content,
    const std::vector<const bounding*>& children)

    : is_terminal(is_terminal), b(b), content(content), children(children) {}

/* Terminal node constructor */
bounding::bounding(const std::vector<unsigned int>& content)

    : is_terminal(true), content(content) {}

/* Internal node constructor */
bounding::bounding(const box* b, const std::vector<const bounding*>& children)

    : is_terminal(false), b(b), children(children) {}


const box* bounding::get_b() const {
    return b;
}



/* Tree-search version of the closest object to the ray r */

/* Auxiliary function */
void bounding::check_box(const ray& r,
    std::stack<unsigned int>& object_stack,
    std::stack<const bounding*>& bounding_stack) const {

    if (is_terminal) {
        for (unsigned int i = 0; i < content.size(); i++) {
            object_stack.push(content.at(i));
        }
    }
    else {
        if (b->measure_distance(r) != infinity) {
            for (unsigned int i = 0; i < children.size(); i++) {
                bounding_stack.push(children.at(i));
            }
        }
    }
}

hit bounding::find_closest_object(const ray& r) {
    /* For all the bounding boxes in bounding::set, we do the following:
       If the bounding box is terminal, add its content to the object stack.
       If it is internal, check the the ray intersects the box.
       If it does, add its children to the bounding stack.
       Then apply the same algorithm to the bounding box stack, until it is empty.
       Finally, apply a linear search for the object of minimum distance on the
       objects of the object stack.
     */

    std::stack<unsigned int> object_stack;
    std::stack<const bounding*> bounding_stack;

    /* Step 1: pass through the set of first-level bounding boxes */
    for (unsigned int i = 0; i < bounding::set.size(); i++) {
        bounding::set.at(i)->check_box(r, object_stack, bounding_stack);
    }

    /* Step 2: apply the same to the bounding box stack */
    while (not bounding_stack.empty()) {
        const bounding* bd = bounding_stack.top();
        bounding_stack.pop();
        bd->check_box(r, object_stack, bounding_stack);
    }

    /* Step 3: search through the objects of the object stack */
    double closest = infinity;
    unsigned int closest_index = -1;
    const unsigned int origin_obj_index = r.get_origin_index();
    const std::vector<const object*>& obj_set = object::set;

    while (not object_stack.empty()) {
        unsigned int i = object_stack.top();
        object_stack.pop();
        if (i != origin_obj_index) {
            const double d = obj_set.at(i)->measure_distance(r);
            if (d < closest) {
                closest = d;
                closest_index = i;
            }
        } 
    }

    /* Finally, return the hit corresponding to the closest object intersected by the ray */
    if (closest_index != (unsigned int) -1) {
        //printf("Returning true: closest_index = %u\n", closest_index);
        return obj_set.at(closest_index)->compute_intersection(r, closest);
    }
    else {
        return hit();
    }
}

/* Returns a non-terminal bounding box (standard, with n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1))
   containing the standard non-terminal bounding boxes bd0 and bd1 */
bounding* containing_bounding(const bounding& bd0, const bounding& bd1) {
    const box* b0 = bd0.get_b();
    const box* b1 = bd1.get_b();
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
   containing the triangles whose indices are in the obj vector */
bounding* containing_objects(const std::vector<unsigned int>& obj) {

    const bounding container = bounding(obj);

    double min_x = infinity;
    double max_x = -infinity;
    double min_y = infinity;
    double max_y = -infinity;
    double min_z = infinity;
    double max_z = -infinity;

    /* Computation of the dimensions of the triangle set */
    for(unsigned int i = 0; i < obj.size(); i++) {
        const rt::vector& p0 = object::set.at(obj.at(i))->get_position();
        const rt::vector p1 = p0 + ((triangle*) object::set.at(obj.at(i)))->get_v1();
        const rt::vector p2 = p0 + ((triangle*) object::set.at(obj.at(i)))->get_v2();

        const double x_max = std::max(p0.x, std::max(p1.x, p2.x));
        const double x_min = std::min(p0.x, std::min(p1.x, p2.x));
        const double y_max = std::max(p0.y, std::max(p1.y, p2.y));
        const double y_min = std::min(p0.y, std::min(p1.y, p2.y));
        const double z_max = std::max(p0.z, std::max(p1.z, p2.z));
        const double z_min = std::min(p0.z, std::min(p1.z, p2.z));

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
        max_x - min_x, max_y - min_y, max_z - min_z
    );

    return new bounding(b, {&container});
}