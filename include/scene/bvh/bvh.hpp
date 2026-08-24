#pragma once

#include "scene/bvh/bounding.hpp"
#include "auxiliary/custom_stack.hpp"

class bvh {

    public:

        std::vector<const bounding*>    bounding_set;
        std::vector<bounding::box_type> box_set;
        unsigned int                    polygons_per_bounding;

        ~bvh() noexcept {
            constexpr unsigned int DEFAULT_STACK_SIZE = 200;
            custom_stack<const bounding*> bd_stack(DEFAULT_STACK_SIZE);
            bd_stack.push(bounding_set);

            while (not bd_stack.empty()) {
                const bounding* bd = bd_stack.pop();
                bd_stack.push(bd->get_children());
                delete bd;
            }
        }
};