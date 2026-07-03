#pragma once

#include "light/vector.hpp"

#include <vector>
#include <span>
#include <cstring>

// #include <iostream>

struct search_tree {

    using bool_type = char;

    // Internal nodes are points that divide the 3D space into 8 regions
    std::vector<rt::vector> internal_nodes;

    // Each index is a vector containing the indices of the points contained in the leaf
    std::vector<std::vector<unsigned int>> leaves;

    // Each index contains a boolean that indicates whether the node is terminal (a leaf) or internal
    std::vector<bool_type> terminal_state;

    // Centroids referenced by the tree
    const std::vector<rt::vector>& means;

    search_tree(const std::vector<rt::vector>& means)
        : means(means) {}

    private:
        void resize_containers(unsigned int n) {
            const unsigned int previous_size = terminal_state.size();
            internal_nodes.resize(n);
            leaves        .resize(n);
            terminal_state.resize(n);
            std::memset(terminal_state.data() + previous_size, 0, (n - previous_size) * sizeof(bool_type));
        }

    public:

        void initial_resize(unsigned int number_of_nodes) {
            resize_containers(number_of_nodes);
        }

        // Increases the size of the containers so that wanted_index is a valide index
        void increase_size(unsigned int wanted_index) {
            const unsigned int current_size = internal_nodes.size();
            if (current_size < wanted_index) {
                const unsigned int target = static_cast<unsigned int>(2.5f * current_size);
                const unsigned int new_size = std::max(target, wanted_index + 1);
                resize_containers(new_size);
            }
        }

        unsigned int search(const rt::vector& v) const;
};

void build_tree(search_tree& tree);