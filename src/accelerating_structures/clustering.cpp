#include "accelerating_structures/clustering.hpp"

#include "accelerating_structures/octree.hpp"
#include "auxiliary/custom_stack.hpp"
#include "parallel/parallel.hpp"

#include <mutex>
#include <queue>

static constexpr unsigned int MAX_NUMBER_OF_ITERATIONS       = 10;
static constexpr unsigned int MIN_FOR_TREE_SEARCH            = 50;
static constexpr unsigned int MIN_NUMBER_OF_POLYGONS_FOR_BOX = 5;
static constexpr unsigned int CARDINAL_OF_BOX_GROUP          = 3;

static constexpr unsigned int DEFAULT_STACK_SIZE = 8192;

static constexpr bool ENABLE_PARALLELISM_FIRST      = true;
static constexpr bool ENABLE_PARALLELISM_ITERATIONS = true;

static constexpr bool DISPLAY_KMEANS = true;

/** K-means clustering algorithm **/


/* Auxiliary function that computes the centroid of a vector of objects */
static rt::vector compute_centroid(const std::vector<element>& elts) {

    if (elts.empty())
        throw std::runtime_error("Error, empty element set\n");

    rt::vector sum = ZERO;
    for (const element& elt : elts)
        sum += elt.get_position();

    return sum / elts.size();
}

static unsigned int linear_search(const std::vector<rt::vector>& means, const rt::vector& v) {

    unsigned int closest_index = 0;
    real distance_to_closest = (means[0] - v).normsq();

    for (unsigned int m = 1; m < means.size(); m++) {
        const real d = (means[m] - v).normsq();
        if (d < distance_to_closest) {
            distance_to_closest = d;
            closest_index = m;
        }
    }

    return closest_index;
}

// Type of search among centroids
enum class search_type {
    Linear, Accelerated
};

/* Auxiliary function that adds each element of old_group to one of the k vectors of new_group,
   according to which of the vectors of means they are the closest
   Returns true if there has been a change of group for at least one element */
static bool assign_to_closest(const std::vector<std::vector<element>>& old_groups, std::vector<std::vector<element>>& new_groups,
    const std::vector<rt::vector>& means) {

    using enum search_type;
    search_type search_type = Linear;
    search_tree tree;
    
    std::cout << "Build tree" << std::endl;
    if (means.size() >= MIN_FOR_TREE_SEARCH) {
        build_tree(means, tree);
        search_type = Accelerated;
    }
    std::cout << "Build tree: done." << std::endl;

    auto search = [&search_type, &means, &tree] (const rt::vector& v) {
        switch (search_type) {
            case Linear:        return linear_search(means, v);
            case Accelerated:   return tree_search(means, tree, v);
        }
    };

    std::mutex mut;
    const unsigned int nb_of_groups = old_groups.size();

    if (nb_of_groups == 1) {
        // First iteration (all elements in the first group)

        const std::vector<element>& old_group = old_groups[0];
        
        std::vector<unsigned int> closest_indices(old_group.size());

        if constexpr (ENABLE_PARALLELISM_FIRST) {
            parallel_for(old_group.size(), [&] (int i) {
                const element& elt = old_group[i];
                closest_indices[i] = search(elt.get_position());
            });
            for (unsigned int i = 0; i < closest_indices.size(); i++)
                new_groups[closest_indices[i]].push_back(old_group[i]);
        }
        else {
            for (const element& elt : old_group) {

                const unsigned int closest_index = search(elt.get_position());
                new_groups[closest_index].push_back(elt);
            }
        }
        return true;
    }
    else {
        // All other iterations, 
        bool change = false;

        if constexpr (ENABLE_PARALLELISM_ITERATIONS) {
            parallel_for(nb_of_groups, [&] (int start, int end) {

                std::vector<unsigned int> closest_indices;

                for (int i = start; i < end; i++) {
                    const auto& group = old_groups[i];
                    //closest_indices.reserve(std::max(closest_indices.size() + group.size(), closest_indices.capacity()));

                    for (const element& elt : group) {
                        const unsigned int closest_index = search(elt.get_position());
                        change = change || (closest_index != static_cast<unsigned int>(i));
                        closest_indices.push_back(closest_index);
                    }
                }

                mut.lock();
                unsigned int j = 0;
                for (int i = start; i < end; i++) {
                    const auto& group = old_groups[i];
                    for (const element& elt : group)
                        new_groups[closest_indices[j++]].push_back(elt);
                }
                mut.unlock();
            });
        }
        else {
            for (unsigned int i = 0; i < nb_of_groups; i++) {

                const auto& group = old_groups[i];

                for (const element& elt : group) {
                    const unsigned int closest_index = search(elt.get_position());
                    change = change || (closest_index != i);
                    new_groups[closest_index].push_back(elt);
                }
            }
        }

        return change;
    }
}

/* Auxiliary function that fills all empty_clusters */
static void fill_empty_clusters(std::vector<std::vector<element>>& groups) {
    static custom_stack<unsigned int> empty_groups(DEFAULT_STACK_SIZE);
    empty_groups.set_empty();
    std::queue<unsigned int> non_empty_groups;
    for (unsigned int n = 0; n < groups.size(); n++) {
        if (groups[n].empty())
            empty_groups.push(n);
        else
            non_empty_groups.push(n);
    }

    while (not empty_groups.empty()) {
        const unsigned int empty     = empty_groups.pop();
        const unsigned int non_empty = non_empty_groups.front();
        
        groups[empty].push_back(groups[non_empty].back());
        groups[non_empty].pop_back();

        non_empty_groups.pop();
        if (not groups[non_empty].empty())
            non_empty_groups.push(non_empty);
    }
}


/* Returns a vector of k vectors of elements representing the k clusters */
static std::vector<std::vector<element>> k_means(const std::vector<element>& obj, const unsigned int k) {

    /* Vectors containing the k initial means */
    std::vector<rt::vector> means(k);

    /* Filling the vector with k elements uniformly distributed along the obj vector */
    const real step = std::max(static_cast<real>(obj.size() / k), 1.0_r);

    const int bound = std::min(static_cast<unsigned int>(obj.size()), k);
    for (int i = 0; i < bound; i++)
        means[i] = obj[static_cast<int>(i * step)].get_position();
    
    std::vector<std::vector<element>> groups(k);
    // std::cout << "assign_to_closest..." << std::endl;
    assign_to_closest({ obj }, groups, means);
    // std::cout << "Done.\nfill_empty_clusters..." << std::endl;
    fill_empty_clusters(groups);
    // std::cout << "Done." << std::endl;

    unsigned int iterations = MAX_NUMBER_OF_ITERATIONS;
    bool change = true;
    
    while (change && iterations != 0) {

        if constexpr (DISPLAY_KMEANS) {
            printf("\rIteration %u / %u",
                MAX_NUMBER_OF_ITERATIONS - iterations, MAX_NUMBER_OF_ITERATIONS);
            fflush(stdout);
        }
        /**/
        else {
            printf("\rOptimizing the data structure... Iteration %u / %u",
                MAX_NUMBER_OF_ITERATIONS - iterations, MAX_NUMBER_OF_ITERATIONS);
            fflush(stdout);
        }
        /**/

        // std::cout << "\nA" << std::endl;

        /* Updating the means */
        means.clear();
        for (const std::vector<element>& group : groups) {
            if (not group.empty())
                means.emplace_back(compute_centroid(group));
        }
        // std::cout << "B" << std::endl;

        /* Re-assigning the objects to the right group */
        std::vector<std::vector<element>> new_groups(k);
        change = assign_to_closest(groups, new_groups, means);

        // std::cout << "C" << std::endl;

        fill_empty_clusters(new_groups);

        // std::cout << "D" << std::endl;

        groups.clear();
        groups.swap(new_groups);

        // std::cout << "E" << std::endl;

        if (change)
            iterations--;
    }

    if constexpr (DISPLAY_KMEANS)
        printf("\r> k_means: %u iteration%s (maximum = %u, n = %zu, k = %u)\n",
            MAX_NUMBER_OF_ITERATIONS - iterations,
            (MAX_NUMBER_OF_ITERATIONS - iterations < 2) ? "" : "s",
            MAX_NUMBER_OF_ITERATIONS, obj.size(), k);

    // std::cout << "F" << std::endl;

    return groups;
}

/* Auxiliary function to create_bounding_hierarchy
   Performs the second step of the algorithm: creates the hierarchy of the terminal boundings */
const bounding* create_hierarchy_from_boundings(std::vector<const bounding*>&& term_nodes) {

    if (term_nodes.size() == 1)
        return term_nodes[0];
    
    if (term_nodes.size() <= CARDINAL_OF_BOX_GROUP)
        return containing_bounding_any(std::move(term_nodes));

    std::vector<element> nodes = element::get_element(term_nodes);

    while (nodes.size() > CARDINAL_OF_BOX_GROUP) {

        // std::cout << "AAA " << nodes.size() << " > " << CARDINAL_OF_BOX_GROUP << std::endl;

        const unsigned int k = 1 + nodes.size() / CARDINAL_OF_BOX_GROUP;

        const std::vector<std::vector<element>> groups = k_means(nodes, k);

        // std::cout << "A" << std::endl;

        std::vector<const bounding*> new_bd_nodes;
        
        unsigned int cpt = 0;
        for (const std::vector<element>& elts : groups) {
            
            if (not elts.empty()) {
                new_bd_nodes.push_back(containing_bounding_any(element::get_content<const bounding*>(elts)));
                cpt++;
            }
        }
        if constexpr (DISPLAY_KMEANS)
            printf("Nodes: %u (empty: %u)\n", cpt, k - cpt);

        nodes.clear();
        nodes = element::get_element(new_bd_nodes);
    }

    if (nodes.size() == 1)
        return nodes[0].get_bounding();
    else
        return containing_bounding_any(element::get_content<const bounding*>(nodes));
}

/* Main function: creates the bounding box hierarchy of a set of objects */

/* Returns a bounding* containing the objects of content, split into a hierarchy of boundings if their number
   exceeds MIN_NUMBER_OF_POLYGONS_FOR_BOX

   The terminal nodes in the hierarchy are split into n / polygons_per_bounding groups of
   polygons_per_bounding polygons on average.
   The non-terminal nodes have CARDINAL_OF_BOX_GROUP children on average.
*/
const bounding* create_bounding_hierarchy(std::vector<const object*>&& content,
    const unsigned int polygons_per_bounding) {

    /* Not enough polygons for it to be worth having a bounding box,
       the bounding here just acts as a container */
    if (content.size() < MIN_NUMBER_OF_POLYGONS_FOR_BOX)
        return new bounding(std::move(content));
    
    /* content fits in one bounding box */
    if (content.size() <= polygons_per_bounding)
        return containing_objects(std::move(content));
   
    /* A hierarchy has to be created */
    if constexpr (DISPLAY_KMEANS)
        printf("\nOptimizing the data structure...\n");
    else {
        printf("\rOptimizing the data structure...");
        fflush(stdout);
    }

    /* Splitting the objects into groups of polygons_per_bounding polygons (on average) */
    const unsigned int k = 1 + content.size() / polygons_per_bounding;
    const std::vector<std::vector<element>> groups = k_means(element::get_element(content), k);

    /** Creating the hierarchy **/

    unsigned int cpt = 0;

    /* Creating terminal nodes */
    std::vector<const bounding*> term_nodes;
    for (const std::vector<element>& group : groups) {
        if (not group.empty()) {
            const bounding* bd = containing_objects(element::get_content<const object*>(group));
            term_nodes.push_back(bd);
            cpt++;
        }
    }
    if constexpr (DISPLAY_KMEANS)
        printf("Nodes: %u (empty: %u)\n", cpt, k - cpt);

    return create_hierarchy_from_boundings(std::move(term_nodes));
}


/** Display function **/

/* Displays the depth of the hierarchy, as well as the minimum, maximum and average arity of each depth */
void display_hierarchy_properties(const bounding* bd0) {

    printf("\r============================= HIERARCHY STATISTICS =============================\n");

    unsigned int level = 0;
    custom_stack<const bounding*> bds;
    bds.push(bd0);

    while (not bds.empty()) {
        // printf("bds.size() = %u\n", bds.size());

        /* Computing min, max and average arity of the nodes on the stack
           If one node is terminal, its arity counts as zero. */
        unsigned int terminal_nodes = 0;
        unsigned int min = -1;
        unsigned int max = 0;
        unsigned int total = 0;
        const unsigned int number_of_nodes = bds.get_size();
        
        static custom_stack<const bounding*> next_bds;
        next_bds.set_empty();

        while (not bds.empty()) {
            const bounding* bd = bds.pop();
            unsigned int arity;
            
            using enum bounding::node_type;
            switch (bd->type) {
                case InternalNode: {
                    const std::span children = bd->get_children();
                    next_bds.push(children);
                    arity = children.size();
                    break;
                }

                case TerminalNode: {
                    terminal_nodes++;
                    arity = bd->get_content().size();
                    break;
                }
            }
            
            if      (arity > max) { max = arity; }
            else if (arity < min) { min = arity; }
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
                    min, max, static_cast<real>(total) / number_of_nodes);
            }
        }
        else {
            if (number_of_nodes == 1) {
                printf("|| Level %u: nodes: %u, arity: %u\n",
                    level, number_of_nodes, total);
            }
            else {
                printf("|| Level %u: nodes: %u, terminal: %u, minimum arity: %u, maximum: %u, average: %lf\n",
                    level, number_of_nodes, terminal_nodes, min, max, static_cast<real>(total) / number_of_nodes);
            }
        }
        
        bds.push(next_bds.get_content());
        next_bds.set_empty();
        
        level++;
    }
    printf("===============================================================================\n");
}