#include "accelerating_structures/octree.hpp"

#include <optional>
#include <stack>
#include <span>
#include <iostream>

#include <cassert>

static constexpr unsigned int MAX_ELTS_PER_LEAF = 10;
static constexpr unsigned int NB_REGIONS = 8;

// Terminology: region (unsigned char) designates the relative index in 0..7 of a child from a given node
// An index (unsigned int) is the index of the node in the tree containers

static inline unsigned int parent_index_of(unsigned int index) {
    return (index - 1u) >> 3u;
}

/* Returns r in 0..7, such that index = 8 * parent_index + r + 1, r in 0..7 */
static inline unsigned char region_of(unsigned int index) {
    return (index - 1u) & 7u;
}

static inline unsigned int index_of_region(unsigned int parent_index, unsigned char relative_region) {
    /******/
    assert(parent_index + 1 < static_cast<unsigned int>(-1) / 8);
    /******/
    return (parent_index << 3u) + relative_region + 1u;
}

static inline unsigned char region_of_point(const rt::vector& v, const rt::vector& node) {
    const bool bx = v.x >= node.x;
    const bool by = v.y >= node.y;
    const bool bz = v.z >= node.z;
    return (bx << 2u) | (by << 1u) | bz;
}

static inline unsigned int compute_subregion_index(const search_tree& tree, const rt::vector& v, const unsigned int index) {
    
    const rt::vector& root = tree.internal_nodes[index];
    const unsigned char region = region_of_point(v, root);
    return index_of_region(index, region);
}

// Each index stores a 3D point p. Dividing the space into 8 regions 0..7
// (1st bit: x < or > p.x; 2nd bit: y < or > p.y; 3rd bit: z < or > p.z)
// E.g. x < p.x (0); y > p.y (1); z > p.z (1), region 011(2) = 3
// The root is stored at index 0. Starting at root p at index ip, each subregion's root (0 <= i <= 7) is stored at index 8 * ip + i + 1.

// Reorganizes the elements between index_min and index_max by region w.r.t. their isobarycenter
// Adds the center to the internal nodes
// Returns an array of 8 unsigned integers containing the starting index of each computed subregion
static std::array<unsigned int, NB_REGIONS + 1> split(search_tree& tree, const unsigned int index_root,
    std::span<unsigned int> elts, const unsigned int index_min) {

    const unsigned int nb_indices = elts.size();
    if (nb_indices == 0)
        throw std::runtime_error("split: empty elts\n");

    // Computing the average coordinates
    rt::vector avg = ZERO;
    for (unsigned int elt : elts)
        avg += tree.means[elt];
    avg /= static_cast<real>(nb_indices);

    tree.increase_size(index_root);
    tree.internal_nodes[index_root] = avg;
    
    // Counting the number of element in each region, and the average of each region
    std::array<unsigned int, NB_REGIONS> nb_elt_region;
    nb_elt_region.fill(0);

    for (unsigned int elt : elts) {
        const rt::vector& v = tree.means[elt];
        const unsigned char region = region_of_point(v, avg);
        nb_elt_region[region]++;
    }

    /***************/
    /**/ std::array<unsigned int, NB_REGIONS> first_index_test;
    /**/ first_index_test.fill(0);
    /**/ for (unsigned int i = 0; i < NB_REGIONS; i++) {
    /**/     unsigned int cpt = index_min;
    /**/     for (unsigned int j = 0; j < i; j++)
    /**/         cpt += nb_elt_region[j];
    /**/     first_index_test[i] = cpt;
    /**/ }
    /***************/
    
    // Computing partial sums
    unsigned int last = index_min;
    std::array<unsigned int, NB_REGIONS>& first_index = nb_elt_region;
    for (unsigned int r = 0; r < NB_REGIONS; r++) {
        const unsigned int nbr = nb_elt_region[r];
        first_index[r] = last;
        last += nbr;
    }

    /***************/
    /**/ for (unsigned int i = 0; i < NB_REGIONS; i++)
    /**/     assert(first_index[i] == first_index_test[i]);
    /***************/

    std::array<unsigned int, NB_REGIONS + 1> output;
    std::copy(first_index.begin(), first_index.end(), output.begin());
    output[NB_REGIONS] = last;

    /***************/
    /**/ for (unsigned int i = 0; i < NB_REGIONS; i++)
    /**/     assert(output[i] == first_index_test[i]);
    /***************/
    
    // Organizing the elements by region
    std::vector<unsigned int> elts_temp(nb_indices);
    
    for (unsigned int elt : elts) {

        const rt::vector& v = tree.means[elt];
        const unsigned char region = region_of_point(v, avg);

        /*****/
        /**/ assert((first_index[region] >= index_min) && (first_index[region] - index_min < elts_temp.size()));
        /*****/

        elts_temp[first_index[region] - index_min] = elt;
        first_index[region]++;
    }

    std::copy(elts_temp.begin(), elts_temp.end(), elts.begin());
    return output;
}


void build_tree(search_tree& tree) {

    const unsigned int size = tree.means.size();
    const unsigned int initial_tree_size = 10 * size;
    tree.initial_resize(initial_tree_size);

    std::vector<unsigned int> elts(size);
    for (unsigned int i = 0; i < size; i++)
        elts[i] = i;

    const unsigned int max_groups = tree.internal_nodes.size();
    /****/
    /**/ assert(max_groups == initial_tree_size);
    /**/ std::cout << "max_groups = " << max_groups << " = " << tree.internal_nodes.size();
    /****/

    std::vector<unsigned int> g1, g2, gs1, gs2, ti1, ti2;
    for (auto v : { &g1, &g2, &gs1, &gs2, &ti1, &ti2 })
        v->reserve(max_groups);

    // First iteration: one group, with all the elements
    g1.push_back(0);
    gs1.push_back(size);
    ti1.push_back(0);

    unsigned int nb_non_terminal_groups = 1;
    bool parity = true;

    /*******/
    /**/ int iter = 0;
    /*******/

    while (nb_non_terminal_groups != 0) {

        /*******/
        /**/ std::cout << "\nbuild_tree: iter " << (iter++) << std::endl;
        /*******/

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
        /*****/
        /**/ std::cout << "build_tree: nb_non_terminal_groups " << nb_non_terminal_groups << std::endl;
        /**/ if (nb_non_terminal_groups - 1 >= groups.size())
        /**/     throw std::runtime_error("loop goes too far\n");
        /*****/
        for (unsigned int g = 0; g < nb_non_terminal_groups; g++) {

            /*******/
            //std::cout << "build_tree: group " << g << std::endl;
            /**/ printf("\rgroup %u", g);
            /*******/

            const unsigned int index_min     = groups[g];
            const unsigned int nb_elts_group = group_size[g];
            const unsigned int tree_index    = tree_indices[g];

            /***************/
            /**/ // Why did I leave this throw?
            /**/ // Is it wrong that tree_index >= tree.internal_nodes.size(), since I do an increase_size right after?
            /**/ if (tree_index >= tree.internal_nodes.size() && not (nb_elts_group <= MAX_ELTS_PER_LEAF)) {
            /**/     std::cout << std::endl;
            /**/     //std::cout << "(nb_elts_group <= MAX_ELTS_PER_LEAF) = " << (nb_elts_group <= MAX_ELTS_PER_LEAF) << std::endl;
            /**/     throw std::runtime_error(
            /**/         (std::string("Erroneous index: ") + std::to_string(tree_index)
            /**/         + " (>= " + std::to_string(tree.internal_nodes.size()) + ")\n").c_str());
            /**/ }
            /***************/
            /**/ if (g >= max_groups) {
            /**/     std::cout << std::endl;
            /**/     throw std::runtime_error(
            /**/         (std::string("Erroneous group: ") + std::to_string(g)
            /**/         + " (>= " + std::to_string(max_groups) + ")\n").c_str());
            /**/ }
            /***************/
            
            if (nb_elts_group <= MAX_ELTS_PER_LEAF) {

                /**/ // std::cout << "yes" << std::endl;

                // Compute leaf
                tree.increase_size(tree_index);

                std::vector<unsigned int>& leaf = tree.leaves[tree_index];
                leaf.reserve(nb_elts_group);
                const std::span<const unsigned int> leaf_elts(elts.data() + index_min, nb_elts_group);
                leaf.insert(leaf.end(), leaf_elts.begin(), leaf_elts.end());
                tree.terminal_state[tree_index] = true;
            }
            else {

                /**/ // std::cout << "no (nb elts = " << nb_elts_group << ")" << std::endl;

                // new_regions has size 9: each of the 8 regions (0..7) is defined by the interval [new_regions[i] .. new_regions[i+1]]
                const std::array<unsigned int, NB_REGIONS + 1> new_regions =
                    split(tree, tree_index, std::span(elts.data() + index_min, nb_elts_group), index_min);

                /***********/
                // /**/ std::cout << " -> after split: new regions = ";
                // /**/ for (unsigned char i = 0; i < 9; i++)
                // /**/     std::cout << new_regions[i] << " ";
                // /**/ std::cout << std::endl;
                /***********/

                // Add the new groups
                /*******/
                /**/ static_assert((NB_REGIONS - 1) + 1 < new_regions.size());
                /*******/
                for (unsigned char i = 0; i < NB_REGIONS; i++) {
                    new_groups.push_back(new_regions[i]);
                    /*****/
                    /**/ if (new_regions[i + 1] < new_regions[i])
                    /**/     throw std::runtime_error("unsigned underflow\n");
                    /*****/
                    new_group_size.push_back(new_regions[i + 1] - new_regions[i]);
                    new_tree_indices.push_back(index_of_region(tree_index, i));
                }
                new_nb_non_term_groups += NB_REGIONS;

                /**/ // std::cout << "iter loop end" << std::endl;
            }

            /**/ // std::cout << "done." << std::endl;
        }

        /*************/
        /**/ if (new_groups.size() != new_group_size.size() || new_groups.size() != new_tree_indices.size()) {
        /**/     std::cout << "ERROR" << std::endl;
        /**/     throw;
        /**/ }
        /**/ std::cout << "\nnew_groups.size() = " << new_groups.size() << std::endl;
        /*************/

        parity = not parity;
        nb_non_terminal_groups = new_nb_non_term_groups;
    }

    /**/ // std::cout << "build_tree: 999" << std::endl;
}

/* Returns the distance (squared) to the closest centroid to the vector v, and the index of the centroid */
static std::pair<real, std::optional<unsigned int>> compute_min_dist_sq(const search_tree& tree,
    const rt::vector& v, const unsigned int index) {

    real min_dist_sq = infinity;
    std::optional<unsigned int> closest_index = std::nullopt;

    for (unsigned int centroid_index : tree.leaves[index]) {

        const rt::vector& centroid = tree.means[centroid_index];
        const real d = (v - centroid).normsq();
        if (d < min_dist_sq) {
            min_dist_sq = d;
            closest_index = centroid_index;
        }
    }

    return { min_dist_sq, closest_index };
}

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

unsigned int search_tree::search(const rt::vector& v) {

    unsigned int index = 0;
    real min_dist = infinity;
    std::optional<unsigned int> closest_centroid_index;

    std::stack<tree_search_info> stack;

    while (not (index == 0 && closest_centroid_index.has_value())) {

        // Go all the way down
        while (not terminal_state[index]) {
            index = compute_subregion_index(*this, v, index);
        }

        // Compute the minimum distance to a point in the current leaf
        if (leaves[index].size() != 0) {

            const auto [ d, ci ] = compute_min_dist_sq(*this, v, index);
            if (d < min_dist) {
                min_dist = d;
                closest_centroid_index = ci;
            }

            // if (verbose) printf("Non-empty leaf. index = %u, min_dist = %f, closest = %u\n", index, min_dist, closest_centroid_index.value());
        }
        // else {
            // if (verbose) printf("Empty leaf. index = %u, min_dist = %f, closest = %u\n", index, min_dist,
            //    closest_centroid_index.value_or(EMPTY_INDEX);
        // }

        // Go back up
        while (index != 0) {

            // if (verbose) printf("Climbing... index = %u\n", index);

            const unsigned int parent_index = parent_index_of(index);
            // index = 8 * new_index + r + 1, r in 0..7
            const unsigned char r = region_of(index);
            const rt::vector& root = internal_nodes[parent_index];

            // Check if the search among the neighborhood was already underway
            unsigned char original_region      = r;
            unsigned char region_to_start_from = 0;
            bool resume = false;
            if (not stack.empty()) {
                const tree_search_info& tsi = stack.top();                
                if (parent_index == tsi.index_stack) {
                    // if (verbose) printf("Resuming: tsi (size %u) index_stack %u, orig region %u, max reg %u\n",
                    //     (unsigned int) stack.size(), tsi.index_stack, tsi.original_region, tsi.max_region_checked);
                    resume = true;
                    original_region      = tsi.original_region;
                    region_to_start_from = tsi.max_region_checked + 1;
                }
            }

            bool need_to_go_back_down = false;

            // Pre-computation for distance checking
            
            const rt::vector d = v - root;
            const unsigned char b = region_of_point(d, ZERO);
            const rt::vector d2 = d * d;

            // Search in all neighbor regions for a point that is closer than min_dist
            // (possible if the region's edge is closer than min_dist)

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

                    index = index_of_region(parent_index, i); // go to region i
                    need_to_go_back_down = true;
                    // if (verbose) printf("Go back down (to index %u)\n", index);
                    break;
                }
            }

            if (need_to_go_back_down)
                break;

            index = parent_index;
            // If resume == true, then we finished examining the neighborhood
            if (resume) /* && not stack.empty()) */ // (resume => stack is not empty)
                stack.pop();
        }
    }

    return closest_centroid_index.value();
}