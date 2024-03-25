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
                if (d < d_cl) {
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
                if (d < distance_to_closest) {
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
const bounding* containing_bounding_two(const bounding*& bd0, const bounding*& bd1) {
    const box* const b0 = bd0->get_b();
    const box* const b1 = bd1->get_b();
    const double bd0l1 = b0->get_l1();
    const double bd0l2 = b0->get_l2();
    const double bd0l3 = b0->get_l3();
    const double bd1l1 = b1->get_l1();
    const double bd1l2 = b1->get_l2();
    const double bd1l3 = b1->get_l3();

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
        return NULL;
    }
    else if (children.size() == 1) {
        return children.at(0);
    }

    double min_x = infinity;
    double max_x = -infinity;
    double min_y = infinity;
    double max_y = -infinity;
    double min_z = infinity;
    double max_z = -infinity;

    /* Computation of the dimensions of the object set */
    for(unsigned int i = 0; i < children.size(); i++) {

        if (children.at(i)->get_b() != NULL) {
            double x_min, x_max, y_min, y_max, z_min, z_max;
            children.at(i)->get_b()->min_max_coord(x_min, x_max, y_min, y_max, z_min, z_max);

            if (x_max > max_x) {max_x = x_max;}
            if (x_min < min_x) {min_x = x_min;}
            if (y_max > max_y) {max_y = y_max;}
            if (y_min < min_y) {min_y = y_min;}
            if (z_max > max_z) {max_z = z_max;}
            if (z_min < min_z) {min_z = z_min;}
        }
    }

    const box* b = new box(
        rt::vector((max_x + min_x) / 2, (max_y + min_y) / 2, (max_z + min_z) / 2),
        rt::vector(1, 0, 0), rt::vector(0, 1, 0),
        max_x - min_x,
        max_y - min_y,
        max_z - min_z
    );
    
    return new bounding(b, children);
}

/* Returns a terminal bounding box (standard, with n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1))
   containing the (finite) objects whose indices are in the obj vector */
const bounding* containing_objects(const std::vector<const object*>& obj) {

    if (obj.size() == 0) {
        printf("Error, empty vector of objects\n");
        return NULL;
    }

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