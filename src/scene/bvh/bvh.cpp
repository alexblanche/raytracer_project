#include "scene/bvh/bvh.hpp"

constexpr unsigned int DEFAULT_STACK_SIZE = 200;

bvh::~bvh() noexcept {
    custom_stack<const bounding*> bd_stack(DEFAULT_STACK_SIZE);
    bd_stack.push(bounding_set);

    while (not bd_stack.empty()) {
        const bounding* bd = bd_stack.pop();
        bd_stack.push(bd->get_children());
        delete bd;
    }
}

min_max_vectors compute_bounding_vectors_bounding_set(
    const std::vector<const bounding*>& set, const bvh& bvh) {
    
    min_max_vectors min_max;

    for (const bounding* const bd : set)
        bvh.get_min_max_coord(bd).update(min_max);
    
    return min_max;
}


/* Tree-search through the bounding boxes */
std::optional<hit> bvh::find_closest_object(const ray& r) const {
    /* For all the bounding boxes in bounding::set, we do the following:
       If the bounding box is terminal, look for the object of minimum distance.
       If it is internal, if the ray intersects the box, add its children to the bounding stack.
       Then apply the same algorithm to the bounding stack, until it is empty.
       Finally, compute the hit associated with the object of minimum distance.
     */

    real distance_to_closest  = infinity;
    const object* closest_obj = nullptr;
    
    static thread_local custom_stack<const bounding*> bounding_stack(DEFAULT_STACK_SIZE);
    bounding_stack.set_empty();

    using enum bounding::node_type;

    const auto update_closest_from_objects = [&] (const bounding* const bd) {

        const std::vector<const object*>& content = bd->get_content();
        
        for (const object* const obj : content) {
            const real d = obj->measure_distance(r);
            if (d < distance_to_closest) {
                distance_to_closest = d;
                closest_obj = obj;
            }
        }
    };

    const auto check_box = [&] (const bounding* const bd) {

        if (bd->box_index != EMPTY_INDEX) {
            const bounding::box_type& b = box_set[bd->box_index];

            if constexpr (std::is_same_v<bounding::box_type, box>) {
                if (reinterpret_cast<const box*>(&b)->is_hit_with_distance(r) >= distance_to_closest)
                    return;
            }
            else if constexpr (std::is_same_v<bounding::box_type, aabb>) {
                if (reinterpret_cast<const aabb*>(&b)->measure_distance(r) >= distance_to_closest)
                    return;
            }
        }

        switch (bd->type) {
            case InternalNode:
                bounding_stack.push(bd->get_children());
                break;
            case TerminalNode:
                update_closest_from_objects(bd);
                break;
        }
    };

    /* Pass through the set of first-level bounding boxes */
    for (const bounding* const bd : bounding_set) {
        check_box(bd);
    }

    /* In order to avoid pushing and then immediately popping an element from bounding_stack,
       we store the last element of bd->children in next_bounding.
       The boolean bd_stored indicates whether we should pop an element, or if one is currently
       stored.
     */
    const bounding* next_bounding = nullptr;
    bool bd_stored = false;

    /* Same as check_box, but the last child is stored in a pointer to avoid pushing and
       immediately popping on the stack */
    const auto check_box_next = [&] (const bounding* const bd) {

        bd_stored = false;

        if (bd->box_index != EMPTY_INDEX) {
            const bounding::box_type& b = box_set[bd->box_index];

            if constexpr (std::is_same_v<bounding::box_type, box>) {
                if (reinterpret_cast<const box*>(&b)->is_hit_with_distance(r) >= distance_to_closest)
                    return;
            }
            else if constexpr (std::is_same_v<bounding::box_type, aabb>) {
                if (reinterpret_cast<const aabb*>(&b)->measure_distance(r) >= distance_to_closest)
                    return;
            }
        }

        switch (bd->type) {

            case InternalNode: {
                const auto& children = bd->get_children();
                const unsigned int last_index = children.size() - 1;
                bounding_stack.push(children.first(last_index));
                next_bounding = children[last_index];
                bd_stored = true;
                break;
            }

            case TerminalNode: {
                update_closest_from_objects(bd);
                break;
            }
        }
    };

    /* Apply the same to the bounding box stack */
    while (bd_stored || (not bounding_stack.empty())) {

        const bounding* bd = bd_stored ? next_bounding : bounding_stack.pop();
        check_box_next(bd);
    }

    /* Finally, return the hit corresponding to the closest object intersected by the ray */
    return (closest_obj != nullptr) ?
          std::optional<hit>(closest_obj->compute_intersection(r, distance_to_closest))
        : std::nullopt;
}