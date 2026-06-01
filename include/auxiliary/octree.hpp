#pragma once

#include "light/vector.hpp"

#include <vector>
#include <span>

struct search_tree {

    using BOOL = char;

    // Internal nodes are points that divide the 3D space into 8 regions
    std::vector<rt::vector> internal_nodes;

    // Each index is a vector containing the indices of the points contained in the leaf
    std::vector<std::vector<unsigned int>> leaves;

    // Each index contains a boolean that indicates whether the node is terminal (a leaf) or internal
    std::vector<BOOL> terminal_state;

    void initial_resize(unsigned int number_of_nodes) {
        internal_nodes.resize(number_of_nodes);
        leaves.resize(number_of_nodes);
        terminal_state.resize(number_of_nodes);
        for (BOOL& state : terminal_state)
            state = false;
    }

    void increase_size(unsigned int wanted_index) {
        const unsigned int current_size = internal_nodes.size();
        if (wanted_index < current_size)
            return;
        const unsigned int new_size = std::max(static_cast<unsigned int>(2.5f * current_size), wanted_index + 1);
        internal_nodes.resize(new_size);
        leaves.resize(new_size);
        terminal_state.resize(new_size);
        std::span terms(terminal_state.data() + current_size, new_size - current_size);
        for (BOOL& state : terms)
            state = false;
    }
};

void build_tree(const std::vector<rt::vector>& means, search_tree& tree);

unsigned int tree_search(const std::vector<rt::vector>& means, const search_tree& tree, const rt::vector& v);// , bool verbose = false);