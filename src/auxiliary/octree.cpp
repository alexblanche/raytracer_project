#include "auxiliary/octree.hpp"

#include <optional>

#include <stack>
#include <iostream>

#define MAX_ELTS_PER_LEAF 10

// Each index stores a 3D point p. Dividing the space into 8 regions 0..7
// (1st bit: x < or > p.x; 2nd bit: y < or > p.y; 3rd bit: z < or > p.z)
// E.g. x < p.x (0); y > p.y (1); z > p.z (1), region 011(2) = 3
// The root is stored at index 0. Starting at root p at index ip, each subregion's root (0 <= i < 7) is stored at index 8 * ip + i + 1.

// Reorganizes the elements between index_min and index_max by region w.r.t. their isobarycenter
// Adds the center to the internal nodes
// Returns an array of 8 unsigned integers containing the starting index of each computed subregion
std::vector<unsigned int> split(const std::vector<rt::vector>& means, search_tree& tree, std::vector<unsigned int>& elts,
    const unsigned int index_root, const unsigned int index_min, const unsigned int index_max) {

    const unsigned int nb_indices = index_max - index_min + 1;
    if (nb_indices == 0) throw;

    // printf("Split\n");

    // printf("nb_indices = %u, min %u, max %u\n", nb_indices, index_min, index_max);

    // Computing the average coordinates
    rt::vector avg(0, 0, 0);
    for (unsigned int i = index_min; i <= index_max; i++) {
        // if (i >= means.size()) printf("i = %u, elts[%u] = %u\n", i, i, elts[i]);
        avg = avg + means[elts[i]];
    }
    avg = avg / nb_indices;

    // printf("O\n");

    tree.internal_nodes[index_root] = avg;
    
    // Counting the number of element in each region, and the average of each region
    // rt::vector avg_region[8];
    unsigned int nb_elt_region[8];
    for (unsigned char r = 0; r < 8; r++) {
        nb_elt_region[r] = 0;
    }
    for (unsigned int i = index_min; i <= index_max; i++) {
        const rt::vector& v = means[elts[i]];
        const unsigned char bx = v.x >= avg.x;
        const unsigned char by = v.y >= avg.y;
        const unsigned char bz = v.z >= avg.z;
        const unsigned char region = (bx << 2) + (by << 1) + bz;
        nb_elt_region[region]++;
        // avg_region[region] = avg_region[region] + v;
    }
    // for (unsigned char i = 0; i < 8; i++) {
    //     if (nb_elt_region[i])
    //         avg_region[i] = avg_region[i] / nb_elt_region[i];
    // }
    
    // Computing partial sums
    unsigned int last = index_min;
    unsigned int* first_index = nb_elt_region;
    for (unsigned char r = 0; r < 8; r++) {
        const unsigned int nbr = nb_elt_region[r];
        first_index[r] = last;
        last += nbr;
    }

    std::vector<unsigned int> output(9);
    for (unsigned char r = 0; r < 8; r++)
        output[r] = first_index[r];
    output[8] = last;

    // for (unsigned char i = 0; i < 8; i++) {
    //     printf("first_index[%u] = %u\n", i, first_index[i]);
    // }

    // Organizing the elements by region
    std::vector<unsigned int> elts_temp(nb_indices);

    // printf("A\n");
    
    for (unsigned int i = index_min; i <= index_max; i++) {

        // printf("i = %u; elts[%u] = %u; means[%u] = (%f, %f, %f)\n", i, i, elts[i], elts[i], means[elts[i]].x, means[elts[i]].y, means[elts[i]].z);
        const rt::vector& v = means[elts[i]];

        const unsigned char bx = v.x >= avg.x;
        const unsigned char by = v.y >= avg.y;
        const unsigned char bz = v.z >= avg.z;
        const unsigned char region = (bx << 2) + (by << 1) + bz;
        // printf("region = %u, first_index[%u] = %u\n", region, region, first_index[region]);
        if (first_index[region] - index_min >= nb_indices) {
            printf("Error index elts_temp\n");
            throw;
        }
        elts_temp[first_index[region] - index_min] = elts[i];
        first_index[region]++;
    }

    // printf("B\n");

    for (unsigned int i = 0; i < nb_indices; i++)
        elts[index_min + i] = elts_temp[i];

    // printf("End split\n");

    return output;
}


void build_tree(const std::vector<rt::vector>& means, search_tree& tree) {

    //
    std::vector<bool> isdone(means.size());
    for (unsigned int i = 0; i < isdone.size(); i++)
        isdone[i] = false;
    //
    
    std::vector<unsigned int> elts(means.size());
    for (unsigned int i = 0; i < means.size(); i++)
        elts[i] = i;

    const unsigned int max_groups = 10 * means.size();
    std::vector<unsigned int> g1(max_groups);
    std::vector<unsigned int> g2(max_groups);
    std::vector<unsigned int> gs1(max_groups);
    std::vector<unsigned int> gs2(max_groups);
    std::vector<unsigned int> ti1(max_groups);
    std::vector<unsigned int> ti2(max_groups);

    // First iteration: one group, with all the elements
    g1[0] = 0;
    gs1[0] = means.size();
    ti1[0] = 0;

    unsigned int nb_non_terminal_groups = 1;
    bool parity = true;

    // printf("Start loop\n");

    unsigned int leaves_done = 0;
    unsigned int points_done = 0;

    // unsigned int depth = 0;

    while (nb_non_terminal_groups) {

        // printf("Loop (depth = %u)\n", depth);
        // depth ++;

        unsigned int new_nb_non_term_groups = 0;

        std::vector<unsigned int>& groups     = (parity) ? g1 : g2;
        std::vector<unsigned int>& new_groups = (parity) ? g2 : g1;

        std::vector<unsigned int>& group_size     = (parity) ? gs1 : gs2;
        std::vector<unsigned int>& new_group_size = (parity) ? gs2 : gs1;

        std::vector<unsigned int>& tree_indices     = (parity) ? ti1 : ti2;
        std::vector<unsigned int>& new_tree_indices = (parity) ? ti2 : ti1;

        // Split all the groups and compute the terminal nodes
        // printf("FOR nb_non_terminal_groups = %u\n", nb_non_terminal_groups);
        // printf("points: %u\n", points_done);
        for (unsigned int g = 0; g < nb_non_terminal_groups; g++) {

            const unsigned int index_min     = groups[g];
            const unsigned int nb_elts_group = group_size[g];
            const unsigned int index_max     = index_min + nb_elts_group - 1;
            const unsigned int tree_index    = tree_indices[g];

            if (tree_index >= tree.internal_nodes.size()) {
                printf("Erroneous index: %u (>= %u) \n", tree_index, (unsigned int) tree.internal_nodes.size());
                throw;
            }
            if (g >= max_groups) {
                printf("Erroneous group: %u (>= %u)\n", g, (unsigned int) max_groups);
                throw;
            }

            // printf("g = %u, tree_index = %u\n", g, tree_index);

            //printf("Y\n");
            
            if (nb_elts_group <= MAX_ELTS_PER_LEAF) {
                // Compute leaf

                // printf("W1 (index %u, nb elts %u, means %u, index_min %u, max %u)\n", index, nb_elts_group,
                //     (unsigned int) means.size(), index_min, index_min + nb_elts_group - 1);

                // printf("Leaves done: %u\n", leaves_done);

                // if (nb_elts_group) printf("Leaves %u\n", (unsigned int) tree.leaves.size());
                // if (nb_elts_group) printf("W1.2 (leaf %u)\n", (unsigned int) tree.leaves[index].size());
                
                // printf("leaf = {}\n");
                // tree.leaves[index] = {};
                // printf("leaf.resize(%u)\n", nb_elts_group);
                tree.leaves[tree_index].resize(nb_elts_group);
                // printf("Resize done\n");
                
                std::vector<unsigned int>& leaf = tree.leaves[tree_index];

                // printf("Printing leaf: (size %llu)\n", leaf.size());
                // for (unsigned int i = 0; i < leaf.size(); i++) {
                //     printf("leaf[%u] = %u\n", i, leaf[i]);
                // }

                // if (nb_elts_group) printf("W1.3 (leaf %u)\n", (unsigned int) leaf.size());
                for (unsigned int i = 0; i < nb_elts_group; i++) {
                    // printf("elts[%u] = %u\n", index_min + i, elts[index_min + i]);
                    // printf("leaf[%u] = %u\n", i, leaf[i]);
                    //tree.leaves[index].push_back(elts[index_min + i]);
                    leaf[i] = elts[index_min + i];

                    //
                    if (isdone[leaf[i]]) {
                        printf("Point already done %u\n", leaf[i]);
                        throw;
                    }
                    // printf("tree_index %u, leaf[%u] = %u\n", tree_index, i, leaf[i]);
                    isdone[leaf[i]] = true;
                    //
                }
                // if (nb_elts_group) printf("W1.5 (leaf %u)\n", (unsigned int) leaf.size());
                tree.terminal_state[tree_index] = true;

                // printf("W2\n\n");
                leaves_done++;
                points_done += nb_elts_group;
                //if (nb_elts_group) printf("points: %u\n", points_done);
            }
            else {

                // printf("split (tree_index %u, index_min %u, index_max %u)\n", tree_index, index_min, index_max);

                // new_regions has size 9: each of the 8 regions (0..7) is defined by the interval [new_regions[i] .. new_regions[i+1]]
                std::vector<unsigned int> new_regions = split(means, tree, elts, tree_index, index_min, index_max);

                //
                // Find duplicates in elts
                for (unsigned int i = 0; i < elts.size(); i++) {
                    for (unsigned int j = 0; j < elts.size(); j++) {
                        if (i != j && elts[i] == elts[j]) {
                            printf("Duplicate found elts[%u] = elts[%u] = %u\n", i, j, elts[i]);
                            throw;
                        }
                    }
                }
                //


                // printf("new regions sizes ");
                // for (unsigned int i = 0; i < 8; i++) {
                //     printf("%u ", new_regions[i+1] - new_regions[i]);
                // }
                // printf("\n");

                // Add the new groups
                const unsigned int ng = new_nb_non_term_groups;
                const unsigned int ni = (tree_index << 3) + 1;
                for (unsigned char i = 0; i < 8; i++) {
                    unsigned int ngi = ng + i;
                    new_groups[ngi]     = new_regions[i];
                    new_group_size[ngi] = new_regions[i + 1] - new_regions[i];
                    new_tree_indices[ngi] = ni + i;
                    //printf("tree_index[%u] = %u (%u * 8 + %u + 1)\n", ngi, ni + i, index, i);
                }
                new_nb_non_term_groups += 8;

                // printf("U\n");
            }
        }

        parity = not parity;
        nb_non_terminal_groups = new_nb_non_term_groups;
    }
    

    //
    for (unsigned int i = 0; i < isdone.size(); i++) {
        if (not isdone[i]) {
            printf("Point not done %u\n", i);
            throw;
        }
    }
    //
}

unsigned int compute_subregion_index(const search_tree& tree, const rt::vector& v, const unsigned int index) {
    
    const rt::vector& root = tree.internal_nodes[index];
    const unsigned char bx = v.x >= root.x;
    const unsigned char by = v.y >= root.y;
    const unsigned char bz = v.z >= root.z;
    const unsigned char region = (bx << 2) + (by << 1) + bz;

    return (index << 3) + region + 1;
}

/* Returns the distance (squared) to the closest centroid to the vector v, and the index of the centroid */
std::pair<real, std::optional<unsigned int>> compute_min_dist_sq(const std::vector<rt::vector>& means, const search_tree& tree,
    const rt::vector& v, const unsigned int index) {

    real min_dist_sq = std::numeric_limits<float>::infinity();
    std::optional<unsigned int> closest_index;

    for (const unsigned int centroid_index : tree.leaves[index]) {

        const rt::vector& centroid = means[centroid_index];
        const real d = (v - centroid).normsq();
        if (d < min_dist_sq) {
            min_dist_sq = d;
            closest_index = centroid_index;
        }
    }

    return std::make_pair(min_dist_sq, closest_index);
}

real distance_sq_to_region(const rt::vector& v, const rt::vector& root, const unsigned char region) {

    const unsigned char bx = v.x >= root.x;
    const unsigned char by = v.y >= root.y;
    const unsigned char bz = v.z >= root.z;

    const unsigned char rx = region & 0x04;
    const unsigned char ry = region & 0x02;
    const unsigned char rz = region & 0x01;

    real d = 0.0f;
    if (bx != rx) {
        const real dx = v.x - root.x;
        d += dx * dx;
    }
    if (by != ry) {
        const real dy = v.y - root.y;
        d += dy * dy;
    }
    if (bz != rz) {
        const real dz = v.z - root.z;
        d += dz * dz;
    }
    return d;
}

struct tree_search_info {
    unsigned int index_stack;
    unsigned char original_region;
    unsigned char max_region_checked;
};

unsigned int tree_search(const std::vector<rt::vector>& means, const search_tree& tree, const rt::vector& v) {

    unsigned int index = 0;
    real min_dist = std::numeric_limits<float>::infinity();
    std::optional<unsigned int> closest_centroid_index;

    std::stack<tree_search_info> stack;

    unsigned int iter = 0;
    // printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");

    while (not (index == 0 && closest_centroid_index.has_value())) {

        iter++;

        if (iter == 1000) {
            printf("Infinite loop\n");
            throw;
        }

        // Go down
        // printf("Go down (index = %u)\n", index);
        //
        unsigned int loop_temp_cpt = 0;
        //
        while (not tree.terminal_state[index]) {

            // printf("Going down... index = %u\n", index);

            index = compute_subregion_index(tree, v, index);

            //
            loop_temp_cpt++;
            if (loop_temp_cpt > 1000) {
                printf("Infinie loop (2)\n");
                throw;
            }
            //
        }

        if (tree.leaves[index].size()) {

            const std::pair<real, std::optional<unsigned int>> p = compute_min_dist_sq(means, tree, v, index);
            const real d = p.first;
            const std::optional<unsigned int>& ci = p.second;

            if (d < min_dist) {
                min_dist = d;
                closest_centroid_index = ci;
            }

            // printf("Non-empty leaf. index = %u, min_dist = %f, closest = %u\n", index, min_dist, closest_centroid_index.value());
        }
        // else {
        //     printf("Empty leaf. index = %u, min_dist = %f, closest = %u\n", index, min_dist,
        //         closest_centroid_index.has_value() ? closest_centroid_index.value() : (unsigned int) (-1));
        // }

        // Go back up
        // printf("Go back up\n");
        while (index != 0) {
            // printf("Climbing... index = %u\n", index);
            const unsigned int parent_index = (index - 1) >> 3;
            // index = 8 * new_index + r + 1, r in 0..7
            const unsigned char r = (index - 1) & 0x07;
            const rt::vector& root = tree.internal_nodes[parent_index];

            unsigned char original_region = r;
            unsigned char region_to_start_from = 0;
            bool resume = false;
            if (not stack.empty()) {
                const tree_search_info& tsi = stack.top();
                // printf("tsi: (size %u) index_stack %u, orig region %u, max reg %u\n", (unsigned int) stack.size(), tsi.index_stack, tsi.original_region, tsi.max_region_checked);
                
                original_region = tsi.original_region;
                if (parent_index == tsi.index_stack) {
                    resume = true;
                    region_to_start_from = tsi.max_region_checked + 1;
                }
            }

            bool gobackdown = false;
            
            for (unsigned char i = region_to_start_from; i < 8; i++) {

                if (i == r || i == original_region) continue;

                const real di = distance_sq_to_region(v, root, i);
                if (di < min_dist) {
                    // Search in this region

                    // Update the stack
                    if (resume) {
                        // Update the current head
                        tree_search_info& tsi = stack.top();
                        tsi.max_region_checked = i;
                    }
                    else {
                        // Add a new element
                        tree_search_info tsi;
                        tsi.index_stack = parent_index;
                        tsi.max_region_checked = i;
                        tsi.original_region = r;
                        stack.push(tsi);
                    }

                    index = (parent_index << 3) + i + 1; // go to region i
                    gobackdown = true;
                    // printf("Go back down (to index %u)\n", index);
                    break;
                }
            }

            if (gobackdown) break;

            index = parent_index;
            if (resume && not stack.empty()) stack.pop();
        }
    }

    printf("Iter %u\n", iter);

    return closest_centroid_index.value();
}