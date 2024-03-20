#pragma once

#include "object.hpp"
#include "box.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"

#include <stack>
#include <vector>

class bounding {
    
    private:
    
        /*
            The search for the intersection point between the ray and the scene will now be performed with a tree-search.
            A bounding box is either a terminal node (leaf), that contains a stack of indices of objects (in object::set),
            or contains a pointer to a box and a stack of indices of bounding boxes contained in said box.
        */

        bool is_terminal;

        /* Bounding box */
        const box* const b;

        /* If the node is terminal: indices of the objects contained in the box */
        std::vector<const object*> content;

        /* If the node is internal: bounding boxes contained in the box */
        std::vector<const bounding*> children;


    public:

        /* Constructors */

        bounding();
        
        bounding(const bool is_terminal, const box* b, const std::vector<const object*>& content,
            const std::vector<const bounding*>& children);

        /* Container node constructor (only for first-level non-triangle objects) */
        bounding(const std::vector<const object*>& content);

        /* Terminal node constructor (with a bounding box, containing triangles) */
        bounding(const std::vector<const object*>& content, const box* b);

        /* Internal node constructor */
        bounding(const box* const b, const std::vector<const bounding*>& children);

        /* Accessors */

        inline const box* get_b() const {
            return b;
        }

        inline const std::vector<const object*>& get_content() const {
            return content;
        }

        inline const std::vector<const bounding*>& get_children() const {
            return children;
        }

        /* Auxiliary function to scene::find_closest_object_bounding :
           Places the children of the bounding on the bounding_stack if the box is hit,
           or determines the closest to the objects from the content if the bounding is terminal
           (if it is closest than the current closest_object, at a distance distance_to_closest,
           in which case the two variables are overwritten)
        */
        void check_box(const ray& r,
            double& distance_to_closest, const object*& closest_object,
            std::stack<const bounding*>& bounding_stack) const;

        /* Same as check_box, but the last child is stored in a pointer to avoid pushing and
           immediately popping on the stack */
        void check_box_next(const ray& r,
            double& distance_to_closest, const object*& closest_object,
            std::stack<const bounding*>& bounding_stack,
            bool& bd_stored, const bounding*& next_bounding) const;
};

/* Returns a bounding box (standard, with n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1))
   containing the bounding boxes bd0 and bd1 */
bounding* containing_bounding_two(const bounding& bd0, const bounding& bd1);

/* Returns a non-terminal bounding box (standard, with n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1))
   containing the standard non-terminal bounding boxes in the children vector */
bounding* containing_bounding_any(const vector<const bounding*>& children);

/* Returns a bounding box (standard, with n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1))
   containing the objects whose indices are in the obj vector */
bounding* containing_objects(const std::vector<const object*>& obj);


