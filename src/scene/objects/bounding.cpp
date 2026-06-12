#include "scene/objects/bounding.hpp"

#include "scene/objects/triangle.hpp"

#include <optional>


/* The vector n3 is taken as the cross product of n1 and n2 */
bounding::bounding(const bool is_terminal, const box* b,
    std::vector<const object*>&& content,
    std::vector<const bounding*>&& children)

    : is_terminal(is_terminal), b(b),
      content(std::move(content)), children(std::move(children)) {}

/* Constructor for terminal nodes: container node (for first-level non-triangle objects) if no box provided,
   or terminal node with a bounding box, containing triangles */
bounding::bounding(std::vector<const object*>&& content, const box* b)

    : is_terminal(true), b(b != nullptr ? std::optional(b) : std::nullopt),
      content(std::move(content)) {}

/* Internal node constructor */
bounding::bounding(std::vector<const bounding*>&& children, const box* b)

    : is_terminal(false), b(b),
      children(std::move(children)) {}



/* Tree-search version of the closest object to the ray r */

/* Auxiliary function to scene::find_closest_object_bounding :
   Places the children of the bounding on the bounding_stack if the box is hit,
   or determines the closest to the objects from the content if the bounding is terminal
   (if it is closest than the current closest_object, at a distance distance_to_closest,
   in which case the two variables are overwritten)
*/
void bounding::check_box(const ray& r,
    // out parameters
    real& distance_to_closest, const object*& closest_object,
    custom_stack<const bounding*>& bounding_stack
    ) const {

    if (b.has_value() && not b.value()->is_hit_by(r))
        return;

    if (not is_terminal) {
        bounding_stack.push(children);
        return;
    }
        
    real d_closest       = distance_to_closest;
    const object* cl_obj = closest_object;
    
    for (const object* const obj : content) {
        const real d = obj->measure_distance(r);
        if (d < d_closest) {
            d_closest = d;
            cl_obj = obj;
        }
    }

    distance_to_closest = d_closest;
    closest_object      = cl_obj;
}

/* Same as check_box, but the last child is stored in a pointer to avoid pushing and
   immediately popping on the stack */
void bounding::check_box_next(const ray& r,
    // out parameters
    real& distance_to_closest, const object*& closest_object,
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

    real d_closest       = distance_to_closest;
    const object* cl_obj = closest_object;

    for (const object* obj : content) {
        const real d = obj->measure_distance(r);
        if (d < d_closest) {
            d_closest = d;
            cl_obj = obj;
        }
    }

    distance_to_closest = d_closest;
    closest_object      = cl_obj;
}
