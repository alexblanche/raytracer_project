#include "scene/objects/box.hpp"
#include "scene/objects/triangle.hpp"
#include "scene/objects/bounding.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"

#include<limits>
numeric_limits<real> realbd;
const real infinity = realbd.infinity();

#include <optional>

#include <stack>
#include <vector>

bounding::bounding()
    : is_terminal(true), b(std::nullopt) {}

/* The vector n3 is taken as the cross product of n1 and n2 */
bounding::bounding(const bool is_terminal, const box* b, const std::vector<const object*>& content,
    const std::vector<const bounding*>& children)

    : is_terminal(is_terminal), b(b), content(content), children(children) {}

/* Container node constructor (only for first-level non-triangle objects) */
bounding::bounding(const std::vector<const object*>& content)

    : is_terminal(true), b(std::nullopt), content(content) {}

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
    real& distance_to_closest, std::optional<const object*>& closest_object,
    std::stack<const bounding*>& bounding_stack) const {

    real d_cl = distance_to_closest;
    std::optional<const object*> cl_obj = closest_object;

    if (is_terminal) {
        if (not b.has_value() || b.value()->is_hit_by(r)) {
            
            for (const object* obj : content) {
                const std::optional<real> d = obj->measure_distance(r);
                if (d.has_value() && d.value() < d_cl) {
                    d_cl = d.value();
                    cl_obj = obj;
                }
            }

        }
    }
    else {
        if (b.value()->is_hit_by(r)) {
            for (const bounding* bd : children) {
                bounding_stack.push(bd);
            }
        }
    }

    distance_to_closest = d_cl;
    closest_object = cl_obj;
}

/* Same as check_box, but the last child is stored in a pointer to avoid pushing and
   immediately popping on the stack */
void bounding::check_box_next(const ray& r,
    real& distance_to_closest, std::optional<const object*>& closest_object,
    std::stack<const bounding*>& bounding_stack,
    bool& bd_stored, const bounding*& next_bounding) const {

    if (is_terminal) {
        if (not b.has_value() || b.value()->is_hit_by(r)) {

            for (const object* obj : content) {
                const std::optional<real> d = obj->measure_distance(r);
                if (d.has_value() && d.value() < distance_to_closest) {
                    distance_to_closest = d.value();
                    closest_object = obj;
                }
            }

        }
        bd_stored = false;
    }
    else {
        if (b.value()->is_hit_by(r)) {
            const size_t last_index = children.size() - 1;
            for (size_t i = 0; i < last_index; i++) {
                bounding_stack.push(children[i]);
            }
            // Last element of children
            next_bounding = children[last_index];
            bd_stored = true;
        }
        else {
            bd_stored = false;
        }
    }
}



/* Returns a non-terminal bounding box (standard, with n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1))
   containing the standard non-terminal bounding boxes bd0 and bd1 */
const bounding* containing_bounding_two(const bounding*& bd0, const bounding*& bd1) {
    const box* const b0 = bd0->get_b().value();
    const box* const b1 = bd1->get_b().value();
    const real bd0l1 = b0->get_l1();
    const real bd0l2 = b0->get_l2();
    const real bd0l3 = b0->get_l3();
    const real bd1l1 = b1->get_l1();
    const real bd1l2 = b1->get_l2();
    const real bd1l3 = b1->get_l3();

    const rt::vector pos0 = b0->get_position();
    const rt::vector pos1 = b1->get_position();
    const rt::vector position(
        (pos1.x + bd1l1 + (pos0.x - bd0l1)) / 2,
        (pos1.y + bd1l2 + (pos0.y - bd0l2)) / 2,
        (pos1.z + bd1l3 + (pos0.z - bd0l3)) / 2);

    const box* b = new box(position,
        rt::vector(1,0,0), rt::vector(0,1,0),
        (pos1.x - pos0.x) + bd0l1 + bd1l1,
        (pos1.y - pos0.y) + bd0l2 + bd1l2,
        (pos1.z - pos0.z) + bd0l3 + bd1l3);
    
    return new bounding(b, {bd0, bd1});
}

/* Returns a non-terminal bounding box (standard, with n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1))
   containing the standard non-terminal bounding boxes in the children vector */
const bounding* containing_bounding_any(const vector<const bounding*>& children) {

    if (children.size() == 0) {
        printf("Error, empty vector of children bounding boxes\n");
        return nullptr;
    }
    else if (children.size() == 1) {
        return children[0];
    }

    real min_x = infinity;
    real max_x = -infinity;
    real min_y = infinity;
    real max_y = -infinity;
    real min_z = infinity;
    real max_z = -infinity;

    /* Computation of the dimensions of the object set */
    for(const bounding* bd : children) {

        if (bd->get_b().has_value()) {
            
            const min_max_coord mmc = bd->get_b().value()->get_min_max_coord();

            if (mmc.max_x > max_x) { max_x = mmc.max_x; }
            if (mmc.min_x < min_x) { min_x = mmc.min_x; }
            if (mmc.max_y > max_y) { max_y = mmc.max_y; }
            if (mmc.min_y < min_y) { min_y = mmc.min_y; }
            if (mmc.max_z > max_z) { max_z = mmc.max_z; }
            if (mmc.min_z < min_z) { min_z = mmc.min_z; }
        }
    }

    const box* b = new box(
        rt::vector((max_x + min_x) / 2.0f, (max_y + min_y) / 2.0f, (max_z + min_z) / 2.0f),
        rt::vector(1, 0, 0), rt::vector(0, 1, 0),
        max_x - min_x,
        max_y - min_y,
        max_z - min_z
    );
    
    return new bounding(b, children);
}

/* Returns a terminal bounding box (standard, with n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1))
   containing the (finite) objects whose indices are in the obj vector */
const bounding* containing_objects(const std::vector<const object*>& objs) {

    real min_x = infinity;
    real max_x = -infinity;
    real min_y = infinity;
    real max_y = -infinity;
    real min_z = infinity;
    real max_z = -infinity;

    /* Computation of the dimensions of the object set */
    for(const object* obj : objs) {
        const min_max_coord mmc = obj->get_min_max_coord();

        if (mmc.max_x > max_x) { max_x = mmc.max_x; }
        if (mmc.min_x < min_x) { min_x = mmc.min_x; }
        if (mmc.max_y > max_y) { max_y = mmc.max_y; }
        if (mmc.min_y < min_y) { min_y = mmc.min_y; }
        if (mmc.max_z > max_z) { max_z = mmc.max_z; }
        if (mmc.min_z < min_z) { min_z = mmc.min_z; }
    }

    const box* b = new box(
        rt::vector((max_x + min_x) / 2.0f, (max_y + min_y) / 2.0f, (max_z + min_z) / 2.0f),
        rt::vector(1, 0, 0), rt::vector(0, 1, 0),
        max_x - min_x,
        max_y - min_y,
        max_z - min_z
    );

    return new bounding(objs, b);
}