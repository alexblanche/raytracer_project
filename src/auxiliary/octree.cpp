#include "auxiliary/octree.hpp"

#include <optional>
#include <stack>
#include <span>
#include <iostream>

static constexpr unsigned int MAX_ELTS_PER_LEAF = 10;
static constexpr unsigned int NB_REGIONS = 8;

// Each index stores a 3D point p. Dividing the space into 8 regions 0..7
// (1st bit: x < or > p.x; 2nd bit: y < or > p.y; 3rd bit: z < or > p.z)
// E.g. x < p.x (0); y > p.y (1); z > p.z (1), region 011(2) = 3
// The root is stored at index 0. Starting at root p at index ip, each subregion's root (0 <= i < 7) is stored at index 8 * ip + i + 1.

// Reorganizes the elements between index_min and index_max by region w.r.t. their isobarycenter
// Adds the center to the internal nodes
// Returns an array of 8 unsigned integers containing the starting index of each computed subregion
static std::vector<unsigned int> split(const std::vector<rt::vector>& means, search_tree& tree, const unsigned int index_root,
    std::span<unsigned int> elts, const unsigned int index_min) {

    const unsigned int nb_indices = elts.size();
    if (nb_indices == 0)
        throw;

    // Computing the average coordinates
    rt::vector avg;
    for (unsigned int elt : elts)
        avg += means[elt];
    avg *= 1.0_r / static_cast<real>(nb_indices);

    tree.increase_size(index_root);
    tree.internal_nodes[index_root] = avg;
    
    // Counting the number of element in each region, and the average of each region
    std::array<unsigned int, NB_REGIONS> nb_elt_region;
    nb_elt_region.fill(0);

    for (unsigned int elt : elts) {
        const rt::vector& v = means[elt];
        const unsigned char bx = v.x >= avg.x;
        const unsigned char by = v.y >= avg.y;
        const unsigned char bz = v.z >= avg.z;
        const unsigned char region = (bx << 2) + (by << 1) + bz;
        nb_elt_region[region]++;
    }
    
    // Computing partial sums
    unsigned int last = index_min;
    std::array<unsigned int, NB_REGIONS>& first_index = nb_elt_region;
    for (unsigned int r = 0; r < NB_REGIONS; r++) {
        const unsigned int nbr = nb_elt_region[r];
        first_index[r] = last;
        last += nbr;
    }

    std::vector<unsigned int> output(NB_REGIONS + 1);
    output.assign_range(first_index);
    output.back() = last;

    // Organizing the elements by region
    std::vector<unsigned int> elts_temp(nb_indices);
    
    for (unsigned int elt : elts) {

        const rt::vector& v = means[elt];
        const unsigned char bx = v.x >= avg.x;
        const unsigned char by = v.y >= avg.y;
        const unsigned char bz = v.z >= avg.z;
        const unsigned char region = (bx << 2) + (by << 1) + bz;
        elts_temp[first_index[region] - index_min] = elt;
        first_index[region]++;
    }

    std::copy(elts_temp.begin(), elts_temp.end(), elts.begin());
    return output;
}


void build_tree(const std::vector<rt::vector>& means, search_tree& tree) {

    tree.initial_resize(10 * means.size());

    std::vector<unsigned int> elts(means.size());
    for (unsigned int i = 0; i < means.size(); i++)
        elts[i] = i;

    const unsigned int max_groups = tree.internal_nodes.size();
    std::vector<unsigned int> g1, g2, gs1, gs2, ti1, ti2;
    for (auto v : { &g1, &g2, &gs1, &gs2, &ti1, &ti2 })
        v->reserve(max_groups);

    // First iteration: one group, with all the elements
    g1.push_back(0);
    gs1.push_back(means.size());
    ti1.push_back(0);

    unsigned int nb_non_terminal_groups = 1;
    bool parity = true;

    while (nb_non_terminal_groups != 0) {

        unsigned int new_nb_non_term_groups = 0;

        std::vector<unsigned int>& groups     = (parity) ? g1 : g2;
        std::vector<unsigned int>& new_groups = (parity) ? g2 : g1;

        std::vector<unsigned int>& group_size     = (parity) ? gs1 : gs2;
        std::vector<unsigned int>& new_group_size = (parity) ? gs2 : gs1;

        std::vector<unsigned int>& tree_indices     = (parity) ? ti1 : ti2;
        std::vector<unsigned int>& new_tree_indices = (parity) ? ti2 : ti1;

        new_groups.clear();
        new_group_size.clear();
        new_tree_indices.clear();

        // Split all the groups and compute the terminal nodes
        for (unsigned int g = 0; g < nb_non_terminal_groups; g++) {

            const unsigned int index_min     = groups[g];
            const unsigned int nb_elts_group = group_size[g];
            const unsigned int tree_index    = tree_indices[g];

            // if (tree_index >= tree.internal_nodes.size()) {
            //     printf("Erroneous index: %u (>= %zu)\n", tree_index, tree.internal_nodes.size());
            //     throw;
            // }
            // if (g >= max_groups) {
            //     printf("Erroneous group: %u (>= %u)\n", g, max_groups);
            //     throw;
            // }
            
            if (nb_elts_group <= MAX_ELTS_PER_LEAF) {
                // Compute leaf
                tree.increase_size(tree_index);

                std::vector<unsigned int>& leaf = tree.leaves[tree_index];
                leaf.reserve(nb_elts_group);
                const std::span<const unsigned int> leaf_elts(elts.data() + index_min, nb_elts_group);
                leaf.insert(leaf.end(), leaf_elts.begin(), leaf_elts.end());
                tree.terminal_state[tree_index] = true;
            }
            else {

                // new_regions has size 9: each of the 8 regions (0..7) is defined by the interval [new_regions[i] .. new_regions[i+1]]
                std::vector<unsigned int> new_regions = split(means, tree, tree_index, std::span(elts.data() + index_min, nb_elts_group), index_min);

                // Add the new groups
                // const unsigned int ng = new_nb_non_term_groups;
                const unsigned int ni = (tree_index << 3) + 1;
                for (unsigned char i = 0; i < 8; i++) {
                    // unsigned int ngi = ng + i;
                    // new_groups[ngi]     = new_regions[i];
                    // new_group_size[ngi] = new_regions[i + 1] - new_regions[i];
                    // new_tree_indices[ngi] = ni + i;
                    new_groups.push_back(new_regions[i]);
                    new_group_size.push_back(new_regions[i + 1] - new_regions[i]);
                    new_tree_indices.push_back(ni + i);
                }
                new_nb_non_term_groups += 8;
            }
        }

        parity = not parity;
        nb_non_terminal_groups = new_nb_non_term_groups;
    }
}

static unsigned int compute_subregion_index(const search_tree& tree, const rt::vector& v, const unsigned int index) {
    
    const rt::vector& root = tree.internal_nodes[index];
    const unsigned char bx = v.x >= root.x;
    const unsigned char by = v.y >= root.y;
    const unsigned char bz = v.z >= root.z;
    const unsigned char region = (bx << 2) + (by << 1) + bz;

    return (index << 3) + region + 1;
}

/* Returns the distance (squared) to the closest centroid to the vector v, and the index of the centroid */
static std::pair<real, std::optional<unsigned int>> compute_min_dist_sq(const std::vector<rt::vector>& means, const search_tree& tree,
    const rt::vector& v, const unsigned int index) {

    real min_dist_sq = infinity;
    std::optional<unsigned int> closest_index;

    for (const unsigned int centroid_index : tree.leaves[index]) {

        const rt::vector& centroid = means[centroid_index];
        const real d = (v - centroid).normsq();
        if (d < min_dist_sq) {
            min_dist_sq = d;
            closest_index = centroid_index;
        }
    }

    return { min_dist_sq, closest_index };
}

/*
real distance_sq_to_region(const rt::vector& v, const rt::vector& root, const unsigned char region) {

    const bool bx = v.x >= root.x;
    const bool by = v.y >= root.y;
    const bool bz = v.z >= root.z;

    const bool rx = region & 0x04;
    const bool ry = region & 0x02;
    const bool rz = region & 0x01;

    real d = 0.0_r;
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
*/

// Optimized version
static inline real distance_sq_to_region(const rt::vector& d2,
    const unsigned char region, const unsigned char b) {

    const unsigned char r = region ^ b;
    return ((r & 0x04) ? d2.x : 0.0_r)
         + ((r & 0x02) ? d2.y : 0.0_r)
         + ((r & 0x01) ? d2.z : 0.0_r);
}

struct tree_search_info {
    unsigned int index_stack;
    unsigned char original_region;
    unsigned char max_region_checked;
};

unsigned int tree_search(const std::vector<rt::vector>& means, const search_tree& tree, const rt::vector& v) {

    unsigned int index = 0;
    real min_dist = infinity;
    std::optional<unsigned int> closest_centroid_index;

    std::stack<tree_search_info> stack;

    while (not (index == 0 && closest_centroid_index.has_value())) {

        // Go down
        while (not tree.terminal_state[index]) {
            index = compute_subregion_index(tree, v, index);
        }

        if (tree.leaves[index].size() != 0) {

            const auto [ d, ci ] = compute_min_dist_sq(means, tree, v, index);
            if (d < min_dist) {
                min_dist = d;
                closest_centroid_index = ci;
            }

            // if (verbose) printf("Non-empty leaf. index = %u, min_dist = %f, closest = %u\n", index, min_dist, closest_centroid_index.value());
        }
        // else {
            // if (verbose) printf("Empty leaf. index = %u, min_dist = %f, closest = %u\n", index, min_dist,
            //    closest_centroid_index.has_value() ? closest_centroid_index.value() : (unsigned int) (-1));
        // }

        // Go back up
        while (index != 0) {
            // if (verbose) printf("Climbing... index = %u\n", index);
            const unsigned int parent_index = (index - 1) >> 3;
            // index = 8 * new_index + r + 1, r in 0..7
            const unsigned char r = (index - 1) & 0x07;
            const rt::vector& root = tree.internal_nodes[parent_index];

            unsigned char original_region = r;
            unsigned char region_to_start_from = 0;
            bool resume = false;
            if (not stack.empty()) {
                const tree_search_info& tsi = stack.top();                
                if (parent_index == tsi.index_stack) {
                    // if (verbose) printf("Resuming: tsi (size %u) index_stack %u, orig region %u, max reg %u\n",
                    //     (unsigned int) stack.size(), tsi.index_stack, tsi.original_region, tsi.max_region_checked);
                    resume = true;
                    original_region = tsi.original_region;
                    region_to_start_from = tsi.max_region_checked + 1;
                }
            }

            bool gobackdown = false;

            // Pre-computation for distance checking
            
            const rt::vector d = v - root;
            const unsigned char b = ((d.x >= 0.0_r) << 2) + ((d.y >= 0.0_r) << 1) + (d.z >= 0.0_r);
            const rt::vector d2(d.x * d.x, d.y * d.y, d.z * d.z);

            for (unsigned char i = region_to_start_from; i < 8; i++) {

                if (i == r || i == original_region)
                    continue;

                const real di = distance_sq_to_region(d2, i, b);
                // if (verbose) printf("Neighbor region %u (%u): distance %f\n", i, 8 * parent_index + i + 1, di);

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
                        const tree_search_info tsi = {
                            .index_stack        = parent_index,
                            .original_region    = r,
                            .max_region_checked = i
                        };
                        stack.push(tsi);
                    }

                    index = (parent_index << 3) + i + 1; // go to region i
                    gobackdown = true;
                    // if (verbose) printf("Go back down (to index %u)\n", index);
                    break;
                }
            }

            if (gobackdown)
                break;

            index = parent_index;
            if (resume && not stack.empty())
                stack.pop();
        }
    }

    return closest_centroid_index.value();
}