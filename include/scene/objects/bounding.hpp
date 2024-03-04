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
        const box* b;

        /* If the node is terminal: indices of the objects contained in the box */
        std::vector<unsigned int> content;

        /* If the node is internal: bounding boxes contained in the box */
        std::vector<const bounding*> children;


    public:

        /* Static container of the first-level bounding boxes */
        static std::vector<const bounding*> set;

        /* Constructors */

        bounding();
        
        bounding(const bool is_terminal, const box* b, const std::vector<unsigned int>& content, const std::vector<const bounding*>& children);

        /* Container node constructor (only for first-level non-triangle objects) */
        bounding(const std::vector<unsigned int>& content);

        /* Terminal node constructor (with a bounding box, containing triangles) */
        bounding(const std::vector<unsigned int>& content, const box* b);

        /* Internal node constructor */
        bounding(const box* b, const std::vector<const bounding*>& children);

        /* Accessors */

        const box* get_b() const {
            return b;
        }

        const std::vector<unsigned int>& get_content() const {
            return content;
        }

        const std::vector<const bounding*>& get_children() const {
            return children;
        }
        

        void check_box(const ray& r,
            double& closest, unsigned int& closest_index,
            std::stack<const bounding*>& bounding_stack) const;

        static hit find_closest_object(const ray& r);
};

/* Returns a bounding box (standard, with n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1))
   containing the bounding boxes bd0 and bd1 */
bounding* containing_bounding(const bounding& bd0, const bounding& bd1);

/* Returns a bounding box (standard, with n1 = (1, 0, 0), n2 = (0, 1, 0), n3 = (0, 0, 1))
   containing the objects whose indices are in the obj vector */
bounding* containing_objects(const std::vector<unsigned int>& obj);


