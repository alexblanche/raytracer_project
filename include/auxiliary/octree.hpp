#pragma once

#include "light/vector.hpp"

struct search_tree {
    // Internal nodes are points that divide the 3D space into 8 regions
    std::vector<rt::vector> internal_nodes;

    // Each index is a vector containing the indices of the points contained in the leaf
    std::vector<std::vector<unsigned int>> leaves;

    // Each index contains a boolean that indicates whether the node is terminal (a leaf) or internal
    std::vector<bool> terminal_state;

    void resize_tree(const unsigned int number_of_nodes) {
        internal_nodes.resize(number_of_nodes);
        leaves.resize(number_of_nodes);
        terminal_state.resize(number_of_nodes);
        for (unsigned int i = 0; i < terminal_state.size(); i++)
            terminal_state[i] = false;
    }
};

void build_tree(const std::vector<rt::vector>& means, search_tree& tree);

unsigned int tree_search(const std::vector<rt::vector>& means, const search_tree& tree, const rt::vector& v);// , bool verbose = false);