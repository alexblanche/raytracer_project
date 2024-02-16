#include "headers/box.hpp"
#include "headers/bounding.hpp"

#include "../../light/headers/vector.hpp"
#include "../../light/headers/hit.hpp"
#include "../material/headers/material.hpp"

#include<limits>
numeric_limits<double> realbd;
const double infinity = realbd.infinity();

#include <stack>

std::vector<const bounding*> bounding::set;

bounding::bounding()
    : is_terminal(true) {}

/* The vector n3 is taken as the cross product of n1 and n2 */
bounding::bounding(const bool is_terminal, const box* b, const std::stack<unsigned int>& content,
    const std::stack<const bounding*>& children)

    : is_terminal(is_terminal), b(b), content(content), children(children) {}

/* Terminal node constructor */
bounding::bounding(const std::stack<unsigned int>& content)

    : is_terminal(true), content(content) {}

/* Internal node constructor */
bounding::bounding(const box* b, const std::stack<const bounding*>& children)

    : is_terminal(false), b(b), children(children) {}




/* Tree-search version of the closest object to the ray r */

/* Auxiliary function */
void bounding::check_box(const ray& r,
    std::stack<std::stack<unsigned int>>& object_stack,
    std::stack<std::stack<const bounding*>>& bounding_stack) const {

    if (is_terminal) {
        object_stack.push(content);
    }
    else {
        if (b->measure_distance(r) < infinity) {
            bounding_stack.push(children);
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

    std::stack<std::stack<unsigned int>> object_stack;
    std::stack<std::stack<const bounding*>> bounding_stack;

    /* Step 1: pass through the set of first-level bounding boxes */
    for (unsigned int i = 0; i < bounding::set.size(); i++) {
        bounding::set.at(i)->check_box(r, object_stack, bounding_stack);
    }

    /* Step 2: apply the same to the bounding box stack */
    while (not bounding_stack.empty()) {
        std::stack<const bounding*> bd_s = bounding_stack.top();
        bounding_stack.pop();
        while (not bd_s.empty()) {
            bd_s.top()->check_box(r, object_stack, bounding_stack);
            bd_s.pop();
        }
    }

    /* Step 3: search through the objects of the object stack */
    double closest = infinity;
    unsigned int closest_index = -1;
    const unsigned int origin_obj_index = r.get_origin_index();
    const std::vector<const object*>& obj_set = object::set;

    //unsigned int cpt = 0;

    while (not object_stack.empty()) {
        std::stack<unsigned int> ob_s = object_stack.top();
        object_stack.pop();
        while (not ob_s.empty()) {
            const unsigned int i = ob_s.top();
            ob_s.pop();
            //cpt++;
            if (i != origin_obj_index) {
                const double d = obj_set.at(i)->measure_distance(r);
                if (d < closest) {
                    closest = d;
                    closest_index = i;
                }
            } 
        }
    }
    
    //printf("%u", cpt);

    /* Finally, return the hit corresponding to the closest object intersected by the ray */
    if (closest_index != (unsigned int) -1) {
        return obj_set.at(closest_index)->compute_intersection(r, closest);
    }
    else {
        return hit();
    }
}