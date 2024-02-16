#pragma once

#include "object.hpp"
#include "box.hpp"

#include "../../../light/headers/vector.hpp"
#include "../../../light/headers/hit.hpp"
#include "../../material/headers/material.hpp"

#include <stack>

class bounding {
    
    private:
    
        /*
            The search for the intersection point between the ray and the scene will now be performed with a tree-search.
            A bounding box is either a terminal node (leaf), that contains a stack of indices of objects (in object::set),
            or contains a pointer to a box and a stack of indices of bounding boxes contained in said box.
        */

        bool is_terminal;

        /* Bounding box */
        const box* b;

        /* If the node is terminal: indices of the objects contained in the box */
        std::stack<unsigned int> content;

        /* If the node is internal: bounding boxes contained in the box */
        std::stack<const bounding*> children;


    public:

        /* Static container of the first-level bounding boxes */
        static std::vector<const bounding*> set;

        /* Constructors */

        bounding();
        
        bounding(const bool is_terminal, const box* b, const std::stack<unsigned int>& content, const std::stack<const bounding*>& children);

        /* Terminal node constructor */
        bounding(const std::stack<unsigned int>& content);

        /* Internal node constructor */
        bounding(const box* b, const std::stack<const bounding*>& children);
        

        void check_box(const ray& r,
            std::stack<std::stack<unsigned int>>& object_stack,
            std::stack<std::stack<const bounding*>>& bounding_stack) const;

        static hit find_closest_object(const ray& r);
};


