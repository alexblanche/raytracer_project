#include "scene/objects/bounding.hpp"

#include "scene/objects/triangle.hpp"

#include <optional>


/* The vector n3 is taken as the cross product of n1 and n2 */
bounding::bounding(const bool is_terminal, const box* b,
    std::vector<const object*>&& content,
    std::vector<const bounding*>&& children)

    : is_terminal(is_terminal), b(b), content(std::move(content)), children(std::move(children)) {}

/* Container node constructor (only for first-level non-triangle objects) */
bounding::bounding(std::vector<const object*>&& content)

    : content(std::move(content)) {}

/* Terminal node constructor (with a bounding box, containing triangles) */
bounding::bounding(std::vector<const object*>&& content, const box* b)

    : b(b), content(std::move(content)) {}

/* Internal node constructor */
bounding::bounding(const box* b, std::vector<const bounding*>&& children)

    : is_terminal(false), b(b), children(std::move(children)) {}



/* Tree-search version of the closest object to the ray r */

/* Auxiliary function to scene::find_closest_object_bounding :
   Places the children of the bounding on the bounding_stack if the box is hit,
   or determines the closest to the objects from the content if the bounding is terminal
   (if it is closest than the current closest_object, at a distance distance_to_closest,
   in which case the two variables are overwritten)
*/
void bounding::check_box(const ray& r,
    real& distance_to_closest, std::optional<const object*>& closest_object,
    //std::stack<const bounding*>& bounding_stack
    custom_stack<const bounding*>& bounding_stack
    ) const {

    if (b.has_value() && not b.value()->is_hit_by(r))
        return;

    if (not is_terminal) {
        bounding_stack.push(children);
        return;
    }
        
    real d_cl = distance_to_closest;
    std::optional<const object*> cl_obj = closest_object;
    
    for (const object* obj : content) {
        const std::optional<real> d = obj->measure_distance(r);
        if (d.has_value() && d.value() < d_cl) {
            d_cl = d.value();
            cl_obj = obj;
        }
    }

    distance_to_closest = d_cl;
    closest_object = cl_obj;
}

/* Same as check_box, but the last child is stored in a pointer to avoid pushing and
   immediately popping on the stack */
void bounding::check_box_next(const ray& r,
    real& distance_to_closest, std::optional<const object*>& closest_object,
    //std::stack<const bounding*>& bounding_stack,
    custom_stack<const bounding*>& bounding_stack,
    bool& bd_stored, const bounding*& next_bounding) const {

    bd_stored = false;

    if (b.has_value() && not b.value()->is_hit_by(r))
        return;

    if (not is_terminal) {
        const unsigned int last_index = children.size() - 1;
        bounding_stack.push(std::span(children).first(last_index));
        next_bounding = children[last_index];
        bd_stored = true;
        return;
    }

    real d_cl = distance_to_closest;
    std::optional<const object*> cl_obj = closest_object;

    for (const object* obj : content) {
        const std::optional<real> d = obj->measure_distance(r);
        if (d.has_value() && d.value() < d_cl) {
            d_cl = d.value();
            cl_obj = obj;
        }
    }

    distance_to_closest = d_cl;
    closest_object = cl_obj;
}



/* Returns a non-terminal bounding box (standard, with n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1))
   containing the standard non-terminal bounding boxes bd0 and bd1 */
const bounding* containing_bounding_two(const bounding* bd0, const bounding* bd1) {
    const box* const b0 = bd0->get_b().value();
    const box* const b1 = bd1->get_b().value();
    const rt::vector bd0l = b0->get_l();
    const rt::vector bd1l = b1->get_l();

    const rt::vector& pos0 = b0->get_position();
    const rt::vector& pos1 = b1->get_position();

    const rt::vector pb0 = pos0 - bd0l;
    const rt::vector pb1 = pos1 + bd1l;
    const rt::vector position = (pb1 + pb0) / 2;
    const rt::vector l = pb1 - pb0;

    const box* b = new box(position, rt::vector(1,0,0), rt::vector(0,1,0), l.x, l.y, l.z);
    return new bounding(b, {bd0, bd1});
}

/* Returns a non-terminal bounding box (standard, with n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1))
   containing the standard non-terminal bounding boxes in the children vector */
const bounding* containing_bounding_any(std::vector<const bounding*>&& children) {

    if (children.size() == 0) {
        printf("Error, empty vector of children bounding boxes\n");
        return nullptr;
    }
    else if (children.size() == 1) {
        return children[0];
    }

    rt::vector max(-infinity, -infinity, -infinity);
    rt::vector min( infinity,  infinity,  infinity);

    /* Computation of the dimensions of the object set */
    for(const bounding* bd : children) {

        const std::optional<const box*> b = bd->get_b();
        if (not b.has_value())
            continue;

        const min_max_coord mmc = b.value()->get_min_max_coord();

        max = {
            std::max(max.x, mmc.max_x),
            std::max(max.y, mmc.max_y),
            std::max(max.z, mmc.max_z)
        };
        min = {
            std::min(min.x, mmc.min_x),
            std::min(min.y, mmc.min_y),
            std::min(min.z, mmc.min_z)
        };
    }

    const rt::vector center = (max + min) * 0.5f;
    const rt::vector diff = max - min;

    const box* b = new box(center,
        rt::vector(1, 0, 0), rt::vector(0, 1, 0),
        diff.x, diff.y, diff.z
    );
    
    return new bounding(b, std::move(children));
}

/* Returns a terminal bounding box (standard, with n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1))
   containing the (finite) objects whose indices are in the obj vector */
const bounding* containing_objects(std::vector<const object*>&& objs) {

    real min_x =  infinity;
    real max_x = -infinity;
    real min_y =  infinity;
    real max_y = -infinity;
    real min_z =  infinity;
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
        rt::vector((max_x + min_x) / 2.0_r, (max_y + min_y) / 2.0_r, (max_z + min_z) / 2.0_r),
        rt::vector(1, 0, 0), rt::vector(0, 1, 0),
        max_x - min_x,
        max_y - min_y,
        max_z - min_z
    );

    return new bounding(std::move(objs), b);
}