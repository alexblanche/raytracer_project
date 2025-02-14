#include "auxiliary/clustering.hpp"

#include "parallel/parallel.h"
#include <mutex>

#include <vector>
#include <stack>
#include <queue>

#define MAX_NUMBER_OF_ITERATIONS 10

#define MIN_FOR_TREE_SEARCH 50
#define MAX_ELTS_PER_LEAF 5

#define MIN_NUMBER_OF_POLYGONS_FOR_BOX 5
#define CARDINAL_OF_BOX_GROUP 3
#define DISPLAY_KMEANS true

// Macros for parallel k_means
#define PARALLEL_FOR_BEGIN(nb_elements) parallel_for(nb_elements, [&](unsigned int start, unsigned int end){ for(unsigned int n = start; n < end; ++n)
#define PARALLEL_FOR_END()})


/** K-means clustering algorithm **/


/* Auxiliary function that computes the centroid of a vector of objects */
rt::vector compute_centroid(const std::vector<element>& elts) {
    if (elts.empty()) {
        printf("Error, empty element set\n");
        return rt::vector();
    }

    real sum_x = 0;
    real sum_y = 0;
    real sum_z = 0;

    /* Computation of the dimensions of the object set */
    for (element const& elt : elts) {
        
        const rt::vector& pos = elt.get_position();
        sum_x += pos.x;
        sum_y += pos.y;
        sum_z += pos.z;
    }

    const unsigned int k = elts.size();

    return rt::vector(sum_x / k, sum_y / k, sum_z / k);
}

unsigned int linear_search(const std::vector<rt::vector>& means, const rt::vector& v) {

    unsigned int closest_index = 0;
    real distance_to_closest = (means[0] - v).normsq();

    for (unsigned int m = 1; m < means.size(); m++) {
        const real d = (means[m] - v).normsq();
        // const rt::vector dv = means[m] - v;
        // const real d = abs(dv.x) + abs(dv.y) + abs(dv.z);
        if(d < distance_to_closest) {
            distance_to_closest = d;
            closest_index = m;
        }
    }

    return closest_index;
}

struct search_tree {
    // Internal nodes are points that divide the 3D space into 8 regions
    std::vector<rt::vector> internal_nodes;

    // Each index is a vector containing the indices of the points contained in the leaf
    std::vector<std::vector<unsigned int>> leaves;

    // Each index contains a boolean that indicates whether the node is terminal (a leaf) or internal
    std::vector<bool> terminal_state;

    search_tree(const unsigned int number_of_nodes) {
        internal_nodes.resize(number_of_nodes);
        leaves.resize(number_of_nodes);
        terminal_state.resize(number_of_nodes);
        for (unsigned int i = 0; i < terminal_state.size(); i++)
            terminal_state[i] = false;
    }
};

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

    // Computing the average coordinates
    rt::vector avg(0, 0, 0);
    for (unsigned int i = index_min; i <= index_max; i++) {
        avg = avg + means[elts[i]];
    }
    avg = avg / nb_indices;

    tree.internal_nodes[index_root] = avg;
    
    // Counting the number of element in each region, and the average of each region
    rt::vector avg_region[8];
    unsigned int nb_elt_region[8];
    for (unsigned int i = index_min; i <= index_max; i++) {
        const rt::vector& v = means[elts[i]];
        const unsigned char bx = v.x >= avg.x;
        const unsigned char by = v.y >= avg.y;
        const unsigned char bz = v.z >= avg.z;
        const unsigned char region = (bx << 2) + (by << 1) + bz;
        nb_elt_region[region]++;
        avg_region[region] = avg_region[region] + v;
    }
    for (unsigned char i = 0; i < 8; i++)
        avg_region[i] = avg_region[i] / nb_elt_region[i];
    
    // Computing partial sums
    unsigned int last = 0;
    unsigned int* first_index = nb_elt_region;
    for (unsigned char i = 0; i < 8; i++) {
        const unsigned int nbi = nb_elt_region[i];
        first_index[i] = last;
        last += nbi;
    }

    std::vector<unsigned int> output(8);
    for (unsigned char i = 0; i < 8; i++)
        output[i] = first_index[i];

    // Organizing the elements by region
    std::vector<unsigned int> elts_temp(nb_indices);
    
    for (unsigned int i = index_min; i <= index_max; i++) {
        const rt::vector& v = means[elts[i]];
        const unsigned char bx = v.x >= avg.x;
        const unsigned char by = v.y >= avg.y;
        const unsigned char bz = v.z >= avg.z;
        const unsigned char region = (bx << 2) + (by << 1) + bz;
        elts_temp[first_index[region]] = i;
        first_index[region]++;
    }

    for (unsigned int i = index_min; i <= index_max; i++)
        elts[i] = elts_temp[i];

    return output;
}


void build_tree(const std::vector<rt::vector>& means, search_tree& tree) {
    
    std::vector<unsigned int> elts(means.size());
    for (unsigned int i = 0; i < means.size(); i++)
        elts[i] = i;

    std::vector<unsigned int> g1(means.size());
    std::vector<unsigned int> g2(means.size());
    std::vector<unsigned int> tree_index(means.size());
    bool parity = true;

    g1[0] = 0;
    g1[1] = means.size() - 1;
    tree_index[0] = 0;

    unsigned int nb_non_terminal_groups = 1;
    while (nb_non_terminal_groups) {
        std::vector<unsigned int>& groups     = (parity) ? g1 : g2;
        std::vector<unsigned int>& new_groups = (parity) ? g2 : g1;

        // Split all the groups and compute the terminal nodes
        for (unsigned int g = 0; g < nb_non_terminal_groups; g++) {

            const unsigned int index_min = groups[g];
            const unsigned int index_max = groups[g + 1];
            const unsigned int index = tree_index[g];
            
            const unsigned int nb_elts_group = index_max - index_min + 1;
            if (nb_elts_group <= MAX_ELTS_PER_LEAF) {
                // Compute leaf

                std::vector<unsigned int>& leaf = tree.leaves[index];
                leaf.resize(nb_elts_group);
                for (unsigned int i = 0; i < nb_elts_group; i++)
                    leaf[i] = elts[index_min + i];
            }
            else {

                std::vector<unsigned int> new_regions = split(means, tree, elts, index, index_min, index_max);

                // Add the new groups
                unsigned int ng = g << 3;
                const unsigned int ni = (index << 3) + 1;
                for (char i = 0; i < 8; i++) {
                    new_groups[ng + i] = new_regions[i];
                    tree_index[ng + i] = ni + i;
                }
            }
        }
    }
}

unsigned int compute_subregion_index(const search_tree& tree, const rt::vector& v, const unsigned int index) {
    
    const rt::vector& root = tree.internal_nodes[index];
    const char first_bit  = v.x >= root.x;
    const char second_bit = v.y >= root.y;
    const char third_bit  = v.z >= root.z;
    const char region = (first_bit << 2) + (second_bit << 1) + third_bit;

    return (index << 3) + region;
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

real distance_sq_to_region(const rt::vector& v, const rt::vector& root, const char region) {

    const char bx = v.x >= root.x;
    const char by = v.y >= root.y;
    const char bz = v.z >= root.z;

    const char rx = region & 0x04;
    const char ry = region & 0x02;
    const char rz = region & 0x01;

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
    char original_region;
    char max_region_checked;
};

unsigned int tree_search(const std::vector<rt::vector>& means, const search_tree& tree, const rt::vector& v) {

    unsigned int index = 0;
    real min_dist = std::numeric_limits<float>::infinity();
    std::optional<unsigned int> closest_centroid_index;

    std::stack<tree_search_info> stack;

    while (not (index == 0 && closest_centroid_index.has_value())) {

        // Go down
        while (not tree.terminal_state[index]) {
            index = compute_subregion_index(tree, v, index);
        }

        if (tree.leaves[index].size()) {
            // /!\ Make sure it is initialized

            std::pair<real, std::optional<unsigned int>> p = compute_min_dist_sq(means, tree, v, index);
            const real d = p.first;
            const std::optional<unsigned int>& ci = p.second;

            if (d < min_dist) {
                min_dist = d;
                closest_centroid_index = ci;
            }
        }

        // Go back up
        while (index != 0) {
            const unsigned int new_index = (index - 1) >> 3;
            // index = 8 * new_index + r + 1, r in 0..7
            const char r = (index - 1) & 0x07;
            const rt::vector& root = tree.internal_nodes[new_index];

            char original_region = r;
            char region_to_start_from = 0;
            bool resume = false;
            if (not stack.empty()) {
                const tree_search_info& tsi = stack.top();
                original_region = tsi.original_region;
                if (new_index == tsi.index_stack) {
                    resume = true;
                    region_to_start_from = tsi.max_region_checked + 1;
                }
            }

            bool gobackdown = false;
            
            for (char i = region_to_start_from; i < 8; i++) {

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
                        tsi.index_stack = index;
                        tsi.max_region_checked = i;
                        tsi.original_region = r;
                        stack.push(tsi);
                    }

                    index = (new_index << 3) + i + 1; // go to region i
                    gobackdown = true;
                    break;
                }
            }

            if (gobackdown) break;

            index = new_index;
            if (resume) stack.pop();
        }
    }

    return closest_centroid_index.value();
}

// Type of search among centroids
#define LINEAR      true
#define ACCELERATED false

/* Auxiliary function that adds each element of old_group to one of the k vectors of new_group,
   according to which of the vectors of means they are the closest
   Returns true if there has been a change of group for at least one element */
bool assign_to_closest(const std::vector<std::vector<element>>& old_groups, std::vector<std::vector<element>>& new_groups,
    const std::vector<rt::vector>& means) {

    //bool search_type = LINEAR;

    search_tree tree(means.size());
    
    // if (means.size() >= MIN_FOR_TREE_SEARCH) {
        
    //     build_tree(means, tree);
    //     //search_type = ACCELERATED;
    // }

    const unsigned int nb_of_groups = old_groups.size();
    
    mutex mut;

    if (nb_of_groups == 1) {
        // First iteration (all elements in the first group)
        printf("Means size : %llu\n", means.size());

        const std::vector<element>& old_group = old_groups[0];

        unsigned int cpt = 0;

        PARALLEL_FOR_BEGIN(old_group.size()) {
        //for (unsigned int n = 0; n < old_group.size(); n++) {

            //printf("%u\n", cpt);

            const element& elt = old_group[n];
            const rt::vector& v = elt.get_position();

            const unsigned int closest_index =
                //(search_type == LINEAR) ?
                (true) ?
                    linear_search(means, v)
                    :
                    tree_search(means, tree, v);

            // Test
            // const unsigned int closest_index_linear = linear_search(means, v);
            // const unsigned int closest_index_tree   = tree_search(means, tree, v);
            // if (closest_index_linear != closest_index_tree) printf("ERROR: tree search incorrect\n\n");

            mut.lock();
            new_groups[closest_index].push_back(elt);
            cpt++;
            mut.unlock();
        } PARALLEL_FOR_END();

        return true;
    }
    else {
        // All other iterations, 
        bool change = false;
        PARALLEL_FOR_BEGIN(nb_of_groups) {
            for (element const& elt : old_groups[n]) {

                const rt::vector& v = elt.get_position();

                unsigned int closest_index = 0;
                real distance_to_closest = (means[0] - v).normsq();

                for (unsigned int m = 1; m < means.size(); m++) {
                    const real d = (means[m] - v).normsq();
                    if(d < distance_to_closest) {
                        distance_to_closest = d;
                        closest_index = m;
                    }
                }
                
                if (closest_index != n) {
                    change = true;
                }

                mut.lock();
                new_groups[closest_index].push_back(elt);
                mut.unlock();
            }
        } PARALLEL_FOR_END();
        return change;
    }
}

/* Auxiliary function that fills all empty_clusters */
void fill_empty_clusters(std::vector<std::vector<element>>& groups) {
    std::stack<unsigned int> empty_groups;
    std::queue<unsigned int> non_empty_groups;
    for(unsigned int n = 0; n < groups.size(); n++) {
        if (groups[n].empty()) {
            empty_groups.push(n);
        }
        else {
            non_empty_groups.push(n);
        }
    }

    while(not empty_groups.empty()) {
        const unsigned int empty = empty_groups.top();
        const unsigned int non_empty = non_empty_groups.front();
        
        groups[empty].push_back(groups[non_empty].back());
        groups[non_empty].pop_back();
        empty_groups.pop();

        non_empty_groups.pop();
        if (not groups[non_empty].empty()) {
            non_empty_groups.push(non_empty);
        }
    }
}


/* Returns a vector of k vectors of elements representing the k clusters */
std::vector<std::vector<element>> k_means(const std::vector<element>& obj, const unsigned int k) {

    //printf("HOHOHO\n");

    /* Vectors containing the k initial means */
    std::vector<rt::vector> means(k);

    /* Filling the vector with k elements uniformly distributed along the obj vector */
    const real step = std::max((real) (obj.size() / k), (real) 1.0f);

    for (unsigned int i = 0; i < std::min((unsigned int) obj.size(), k); i++) {
        means[i] = obj[(int) (i * step)].get_position();
    }
        
    std::vector<std::vector<element>> groups(k);
    assign_to_closest({obj}, groups, means);
    //printf("WOWOWO\n");
    fill_empty_clusters(groups);
    //printf("HAHAHA\n");

    unsigned int iterations = MAX_NUMBER_OF_ITERATIONS;
    bool change = true;
    
    while (change && iterations != 0) {

        if (DISPLAY_KMEANS) {
            printf("\rIteration %u / %u", MAX_NUMBER_OF_ITERATIONS - iterations, MAX_NUMBER_OF_ITERATIONS);
        }
        else {
            printf("\rOptimizing the data structure... Iteration %u / %u", MAX_NUMBER_OF_ITERATIONS - iterations, MAX_NUMBER_OF_ITERATIONS);
        }
        fflush(stdout);

        /* Updating the means */
        means.clear();
        for (std::vector<element> const& elts : groups) {
            if (not elts.empty()) {
                means.push_back(compute_centroid(elts));
            }
        }

        /* Re-assigning the objects to the right group */
        std::vector<std::vector<element>> new_groups(k);
        change = assign_to_closest(groups, new_groups, means);
        fill_empty_clusters(new_groups);

        groups.clear();
        groups.swap(new_groups);

        if (change) {
            iterations--;
        }
    }

    if (DISPLAY_KMEANS) {
        if (MAX_NUMBER_OF_ITERATIONS - iterations < 2) {
            printf("\r> k_means: %u iteration (maximum = %u, n = %u, k = %u)\n",
            MAX_NUMBER_OF_ITERATIONS - iterations, MAX_NUMBER_OF_ITERATIONS, (unsigned int) obj.size(), k);
        }
        else {
            printf("\r> k_means: %u iterations (maximum = %u, n = %u, k = %u)\n",
            MAX_NUMBER_OF_ITERATIONS - iterations, MAX_NUMBER_OF_ITERATIONS, (unsigned int) obj.size(), k);
        }
        fflush(stdout);
    }

    return groups;
}

/** Auxiliary conversion functions **/

std::vector<element> get_element_vector(const std::vector<const object*>& objs) {
    std::vector<element> elts;
    elts.reserve(objs.size());
    for (const object* obj : objs) {
        elts.push_back(element(obj));
    }
    return elts;
}

std::vector<element> get_element_vector(const std::vector<const bounding*>& objs) {
    std::vector<element> elts;
    elts.reserve(objs.size());
    for (const bounding* bd : objs) {
        elts.push_back(element(bd));
    }
    return elts;
}

std::vector<const object*> get_object_vector(const std::vector<element>& elts) {
    std::vector<const object*> obj;
    obj.reserve(elts.size());
    for (element const& elt : elts) {
        obj.push_back(elt.obj.value());
    }
    return obj;
}

std::vector<const bounding*> get_bounding_vector(const std::vector<element>& elts) {
    std::vector<const bounding*> bds;
    bds.reserve(elts.size());
    for (element const& elt : elts) {
        bds.push_back(elt.bd.value());
    }
    return bds;
}

/* Auxiliary function to create_bounding_hierarchy
   Performs the second step of the algorithm: creates the hierarchy of the terminal boundings */
const bounding* create_hierarchy_from_boundings(const std::vector<const bounding*>& term_nodes) {

    if (term_nodes.size() == 1) {
        return term_nodes[0];
    }
    else if (term_nodes.size() <= CARDINAL_OF_BOX_GROUP) {
        return containing_bounding_any(term_nodes);
    }

    std::vector<element> nodes = get_element_vector(term_nodes);

    //printf("LALA\n");

    while (nodes.size() > CARDINAL_OF_BOX_GROUP) {

        const unsigned int k = 1 + nodes.size() / CARDINAL_OF_BOX_GROUP;

        const std::vector<std::vector<element>> groups = k_means(nodes, k);

        std::vector<const bounding*> new_bd_nodes;
        
        unsigned int cpt = 0;
        for (std::vector<element> const& elts : groups) {
            
            if (not elts.empty()) {
                new_bd_nodes.push_back(containing_bounding_any(get_bounding_vector(elts)));
                cpt ++;
            }
        }
        if (DISPLAY_KMEANS) {
            printf("Nodes: %u (empty: %u)\n", cpt, k - cpt);
            fflush(stdout);
        }

        nodes.clear();
        nodes = get_element_vector(new_bd_nodes);
    }

    if (nodes.size() == 1) {
        return nodes[0].bd.value();
    }
    else {
        return containing_bounding_any(get_bounding_vector(nodes));
    }
}


/* Main function: creates the bounding box hierarchy of a set of objects */

/* Returns a bounding* containing the objects of content, split into a hierarchy of boundings if their number
   exceeds MIN_NUMBER_OF_POLYGONS_FOR_BOX

   The terminal nodes in the hierarchy are split into n / polygons_per_bounding groups of
   polygons_per_bounding polygons on average.
   The non-terminal nodes have CARDINAL_OF_BOX_GROUP children on average.
*/
const bounding* create_bounding_hierarchy(const std::vector<const object*>& content,
    const unsigned int polygons_per_bounding) {

    /* Not enough polygons for it to be worth having a bounding box,
       the bounding here just acts as a container */
    if (content.size() < MIN_NUMBER_OF_POLYGONS_FOR_BOX) {
        return new bounding(content);
    }
    
    /* content fits in one bounding box */
    if (content.size() <= polygons_per_bounding) {
        return containing_objects(content);
    }
   
    /* A hierarchy has to be created */
    if (DISPLAY_KMEANS) {
        printf("\nOptimizing the data structure...\n");
        //printf("LALALALALA\n");
    }
    else {
        printf("\rOptimizing the data structure...");
    }
    fflush(stdout);

    /* Splitting the objects into groups of polygons_per_bounding polygons (on average) */
    const unsigned int k = 1 + content.size() / polygons_per_bounding;
    const std::vector<std::vector<element>> groups = k_means(get_element_vector(content), k);

    //printf("HEHE\n");

    /** Creating the hierarchy **/

    unsigned int cpt = 0;

    /* Creating terminal nodes */
    std::vector<const bounding*> term_nodes;
    for (unsigned int i = 0; i < k; i++) {
        if (not groups[i].empty()) {
            term_nodes.push_back(containing_objects(get_object_vector(groups[i])));
            cpt ++;
        }
    }
    if (DISPLAY_KMEANS) {
        printf("Nodes: %u (empty: %u)\n", cpt, k - cpt);
        fflush(stdout);
    }
    
    return create_hierarchy_from_boundings(term_nodes);
}


/** Display function **/

/* Displays the depth of the hierarchy, as well as the minimum, maximum and average arity of each depth */
void display_hierarchy_properties(const bounding* bd0) {

    printf("============================= HIERARCHY STATISTICS =============================\n");

    unsigned int level = 0;
    std::stack<const bounding*> bds;
    bds.push(bd0);

    while (not bds.empty()) {
        // printf("bds.size() = %u\n", bds.size());

        /* Computing min, max and average arity of the nodes on the stack
           If one node is terminal, its arity counts as zero. */
        unsigned int terminal_nodes = 0;
        unsigned int min = -1;
        unsigned int max = 0;
        unsigned int total = 0;
        const unsigned int number_of_nodes = bds.size();
        std::stack<const bounding*> next_bds;
        while(not bds.empty()) {
            const bounding* bd = bds.top();
            bds.pop();
            unsigned int arity;
            if (bd->is_terminal_bd()) {
                terminal_nodes ++;
                arity = bd->get_content().size();
            }
            else {
                arity = bd->get_children().size();
                for (size_t j = 0; j < arity; j++) {
                    next_bds.push(bd->get_children()[j]);
                }
            }
            if (arity > max) {max = arity;}
            if (arity < min) {min = arity;}
            total += arity;
        }

        /* Displaying the statistics */
        if (terminal_nodes == number_of_nodes) {
            if (number_of_nodes == 1) {
                printf("|| Level %u: nodes: %u, terminal\n", level, number_of_nodes);
                printf("|| Object arity: %u\n", total);
            }
            else {
                printf("|| Level %u: nodes: %u, all terminal\n", level, number_of_nodes);
                printf("|| Minimum object arity: %u, maximum: %u, average: %lf\n",
                    min, max, ((real) total) / number_of_nodes);
            }
        }
        else {
            if (number_of_nodes == 1) {
                printf("|| Level %u: nodes: %u, arity: %u\n",
                    level, number_of_nodes, total);
            }
            else {
                printf("|| Level %u: nodes: %u, terminal: %u, minimum arity: %u, maximum: %u, average: %lf\n",
                    level, number_of_nodes, terminal_nodes, min, max, ((real) total) / number_of_nodes);
            }
        }
        
        // bds = next_bds;
        while (not next_bds.empty()) {
            bds.push(next_bds.top());
            next_bds.pop();
        }

        level++;
    }
    printf("===============================================================================\n");
    fflush(stdout);
}