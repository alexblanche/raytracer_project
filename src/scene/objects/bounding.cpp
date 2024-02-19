#include "headers/box.hpp"
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




/* Tree-search version of the closest object to the ray r */

/* Auxiliary function */
void bounding::check_box(const ray& r,
    std::stack<unsigned int>& object_stack,
    std::stack<const bounding*>& bounding_stack) const {

    if (is_terminal) {
        for (unsigned int i = 0; i < content.size(); i++) {
            //printf("Pushing object of index %u\n", content.at(i));
            object_stack.push(content.at(i));
        }
    }
    else {
        if (b->measure_distance(r) != infinity) {
            for (unsigned int i = 0; i < children.size(); i++) {
                //printf("Pushing one bounding box.\n");
                bounding_stack.push(children.at(i));
            }
        }
        //else {
            //printf("One box was not intersected.\n");
        //}
    }
}

hit bounding::find_closest_object(const ray& r, const unsigned int bounce) {
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

    //printf("Finding_closest_object: bounce = %u\n", bounce);

    //printf("Step 1\n");

    /* Step 1: pass through the set of first-level bounding boxes */
    for (unsigned int i = 0; i < bounding::set.size(); i++) {
        bounding::set.at(i)->check_box(r, object_stack, bounding_stack);
    }

    //printf("Step 2\n");

    /* Step 2: apply the same to the bounding box stack */
    while (not bounding_stack.empty()) {
        const bounding* bd = bounding_stack.top();
        bounding_stack.pop();
        bd->check_box(r, object_stack, bounding_stack);
    }

    //printf("Step 3\n");

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