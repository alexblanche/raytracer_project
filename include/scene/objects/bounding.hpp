#pragma once

#include "scene/objects/box.hpp"
#include "auxiliary/custom_stack.hpp"

class bounding {
    
    public:
    
        /*
            The search for the intersection point between the ray and the scene will now be performed with a tree-search.
            A bounding box is either a terminal node (leaf), that contains a stack of indices of objects (in object::set),
            or contains a pointer to a box and a stack of indices of bounding boxes contained in said box.
        */

        bool is_terminal = true;

        /* Bounding box */
        const box* b = nullptr;

    
    private:

        union node {
            /* If the node is terminal: indices of the objects contained in the box */
            std::vector<const object*> content;
            /* If the node is internal: bounding boxes contained in the box */
            std::vector<const bounding*> children;

            node(std::vector<const object*>&& content)
                : content(std::move(content)) {}

            node(std::vector<const bounding*>&& children)
                : children(std::move(children)) {}

            ~node() {}
        } node;

    public:

        /* Constructor for terminal nodes: container node (for first-level non-triangle objects) if no box provided,
           or terminal node with a bounding box, containing triangles */
        bounding(std::vector<const object*>&& content, const box* b = nullptr);

        /* Internal node constructor */
        bounding(std::vector<const bounding*>&& children, const box* b);

        bounding(const bounding&)            = delete;
        bounding(bounding&&)                 = delete;
        bounding& operator=(const bounding&) = delete;
        bounding& operator=(bounding&&)      = delete;

        ~bounding() {
            if (b != nullptr)
                delete b;
            
            // objects and chidlren are destroyed by the scene destructor
        }

        inline const std::vector<const object*>& get_content() const {
            if (not is_terminal)
                throw std::runtime_error("Getting content of a non-terminal bounding");
            return node.content;
        }

        inline const std::span<const bounding * const> get_children() const {
            return (not is_terminal) ?
                  node.children
                : std::span<const bounding * const> {};
        }

        inline min_max_coord get_min_max_coord() const {
            return (b != nullptr) ?
                  b->get_min_max_coord()
                : empty_set_min_max_coords;
        }

        /* Auxiliary function to scene::find_closest_object_bounding :
           Places the children of the bounding on the bounding_stack if the box is hit,
           or determines the closest to the objects from the content if the bounding is terminal
           (if it is closest than the current closest_object, at a distance distance_to_closest,
           in which case the two variables are overwritten)
        */
        void check_box(const ray& r,
            real& distance_to_closest, const object*& closest_object,
            custom_stack<const bounding*>& bounding_stack
            ) const;

        /* Same as check_box, but the last child is stored in a pointer to avoid pushing and
           immediately popping on the stack */
        void check_box_next(const ray& r,
            real& distance_to_closest, const object*& closest_object,
            custom_stack<const bounding*>& bounding_stack,
            bool& bd_stored, const bounding*& next_bounding) const;
};

template<typename T>
// T should be object or bounding
requires (requires (T x) { { x.get_min_max_coord() } -> std::same_as<min_max_coord>; })
[[nodiscard]] static const bounding* containing_bounding_template(std::vector<const T*>&& set) {

    if (set.size() == 0) {
        printf("Error: creating bounding box of empty set\n");
        return nullptr;
    }
    else if (set.size() == 1) {
        if constexpr (std::is_same_v<T, bounding>)
            return set[0];
        else
            return new bounding({ set[0] });
    }

    rt::vector max(-infinity, -infinity, -infinity);
    rt::vector min( infinity,  infinity,  infinity);

    /* Computation of the dimensions of the object set */
    for(const T* p : set) {
        p->get_min_max_coord().update_min_max_coord(min, max);
    }

    const rt::vector center = (max + min) / 2.0_r;
    const auto [ l1, l2, l3 ] = max - min;

    const box* b = new box(center, rt::vector(1, 0, 0), rt::vector(0, 1, 0), l1, l2, l3);
    return new bounding(std::forward<std::vector<const T*>>(set), b);
}

/* Returns a bounding box (standard, with n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1))
   containing the bounding boxes bd0 and bd1 */
[[nodiscard]] inline const bounding* containing_bounding_two(const bounding* bd0, const bounding* bd1) {
    return containing_bounding_template<bounding>({ bd0, bd1 });
}

/* Returns a non-terminal bounding box (standard, with n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1))
   containing the standard non-terminal bounding boxes in the children vector */
[[nodiscard]] inline const bounding* containing_bounding_any(std::vector<const bounding*>&& children) {
    return containing_bounding_template(std::move(children));
}

/* Returns a bounding box (standard, with n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1))
   containing the objects whose indices are in the obj vector */
[[nodiscard]] inline const bounding* containing_objects(std::vector<const object*>&& obj) {
    return containing_bounding_template(std::move(obj));
}

