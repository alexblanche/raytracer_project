#pragma once

#include "scene/objects/box.hpp"
#include "scene/bvh/aabb.hpp"
#include "auxiliary/custom_stack.hpp"

#include <memory>
#include <vector>

class bounding {

    public:

        using box_type = aabb;

        enum class node_type {
            InternalNode, TerminalNode
        };
    
        /*
            The search for the intersection point between the ray and the scene will now be performed with a tree-search.
            A bounding box is either a terminal node (leaf), that contains a stack of indices of objects (in object::set),
            or contains a pointer to a box and a stack of indices of bounding boxes contained in said box.
        */

        node_type type;

        /* Index of the bounding box in bvh::box_set */
        unsigned int box_index;

        static inline unsigned int cpt = 0;
        
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

            ~node() noexcept {}
        };
    
        node node_;

    public:

        using enum node_type;

        /* Constructor for terminal nodes: container node (for first-level non-triangle objects) if no box provided,
           or terminal node with a bounding box, containing triangles */
        bounding(std::vector<const object*>&& content, unsigned int box_index = EMPTY_INDEX)
            : type(TerminalNode), box_index(box_index), node_(std::move(content)) { cpt++; }

        /* Internal node constructor */
        bounding(std::vector<const bounding*>&& children, unsigned int box_index)
            : type(InternalNode), box_index(box_index), node_(std::move(children)) { cpt++; }

        bounding(const bounding&)            = delete;
        bounding(bounding&&)                 = delete;
        bounding& operator=(const bounding&) = delete;
        bounding& operator=(bounding&&)      = delete;

        // No destructor: objects and children are destroyed by the scene destructor

        inline const std::vector<const object*>& get_content() const {
            switch (type) {
                case InternalNode:
                    throw std::runtime_error("Getting content of a non-terminal bounding");
                case TerminalNode:
                    return node_.content;
                default: throw;
            }
        }

        inline std::span<const bounding * const> get_children() const {
            return (type == InternalNode) ?
                  node_.children
                : std::span<const bounding * const> {};
        }
};