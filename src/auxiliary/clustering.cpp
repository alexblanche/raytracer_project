#include "auxiliary/clustering.hpp"

#include <vector>
#include "scene/objects/object.hpp"
#include "scene/objects/bounding.hpp"

#include "parallel/parallel.h"
/*
#ifdef __unix__
#include <mutex>
#else
#include "mingw.mutex.h"
#endif
*/
#include <mutex>

#include <stack>
#include <queue>

#include<limits>
numeric_limits<double> realclu;
const double infinity = realclu.infinity();

#define MAX_NUMBER_OF_ITERATIONS 10

#define MIN_NUMBER_OF_POLYGONS_FOR_BOX 5
#define CARDINAL_OF_BOX_GROUP 3
#define DISPLAY_KMEANS false

// Macros for parallel k_means
#define PARALLEL_FOR_BEGIN(nb_elements) parallel_for(nb_elements, [&](unsigned int start, unsigned int end){ for(unsigned int n = start; n < end; ++n)
#define PARALLEL_FOR_END()})

/** K-means clustering algorithm **/

/* Auxiliary function that computes the centroid of a vector of objects */
rt::vector compute_centroid(const std::vector<element>& elts) {
    if (elts.size() == 0) {
        printf("Error, empty element set\n");
        return rt::vector(-infinity, -infinity, -infinity);
    }

    double sum_x = 0;
    double sum_y = 0;
    double sum_z = 0;

    /* Computation of the dimensions of the object set */
    for (const element& elt : elts) {
        
        const rt::vector pos = elt.get_position();
        sum_x += pos.x;
        sum_y += pos.y;
        sum_z += pos.z;
    }

    const unsigned int k = elts.size();

    return rt::vector(sum_x / k, sum_y / k, sum_z / k);
}

/* Auxiliary function that adds each element of old_group to one of the k vectors of new_group,
   according to which of the vectors of means they are the closest
   Returns true if there has been a change of group for at least one element */
bool assign_to_closest(const std::vector<std::vector<element>>& old_groups, std::vector<std::vector<element>>& new_groups,
    const std::vector<rt::vector>& means) {

    
    // Former sequential version
    /*
    bool change = false;
    for (unsigned int n = 0; n < old_group.size(); n++) {
        for (unsigned int i = 0; i < old_group[n].size(); i++) {

            const element elt = old_group[n][i];
            const rt::vector v = elt.get_position();

            unsigned int closest_index = 0;
            double distance_to_closest = (means[0] - v).normsq();

            for (unsigned int m = 1; m < means.size(); m++) {
                const double d = (means[m] - v).normsq();
                if(d < distance_to_closest) {
                    distance_to_closest = d;
                    closest_index = m;
                }
            }

            if (closest_index != n) {
                change = true;
            }
            new_group[closest_index].push_back(elt);
        }
    }
    */


    // Parallel version
    const unsigned int nb_of_groups = old_groups.size();
    
    mutex m;

    if (nb_of_groups == 1) {
        // First iteration (all elements in the first group)
        PARALLEL_FOR_BEGIN(old_groups[0].size()) {

            const element elt = old_groups[0][n];
            const rt::vector v = elt.get_position();

            unsigned int closest_index = 0;
            double distance_to_closest = (means[0] - v).normsq();

            for (unsigned int m = 1; m < means.size(); m++) {
                const double d = (means[m] - v).normsq();
                if(d < distance_to_closest) {
                    distance_to_closest = d;
                    closest_index = m;
                }
            }
            m.lock();
            new_groups[closest_index].push_back(elt);
            m.unlock();
        } PARALLEL_FOR_END();

        return true;
    }
    else {
        // All other iterations, 
        bool change = false;
        PARALLEL_FOR_BEGIN(nb_of_groups) {
            for (element const& elt : old_groups[n]) {

                const rt::vector v = elt.get_position();

                unsigned int closest_index = 0;
                double distance_to_closest = (means[0] - v).normsq();

                for (unsigned int m = 1; m < means.size(); m++) {
                    const double d = (means[m] - v).normsq();
                    if(d < distance_to_closest) {
                        distance_to_closest = d;
                        closest_index = m;
                    }
                }
                
                if (closest_index != n) {
                    change = true;
                }

                m.lock();
                new_groups[closest_index].push_back(elt);
                m.unlock();
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

    /* Vectors containing the k initial means */
    std::vector<rt::vector> means(k);

    /* Filling the vector with k elements uniformly distributed along the obj vector */
    const double step = std::max((double) (obj.size() / k), 1.0);

    for (unsigned int i = 0; i < std::min((unsigned int) obj.size(), k); i++) {
        means[i] = obj[(int) (i * step)].get_position();
    }
    
    std::vector<std::vector<element>> groups(k);
    assign_to_closest({obj}, groups, means);
    fill_empty_clusters(groups);

    unsigned int iterations = MAX_NUMBER_OF_ITERATIONS;
    bool change = true;
    
    while (change && iterations != 0) {

        if (DISPLAY_KMEANS) {
            printf("\rIteration %u / %u", MAX_NUMBER_OF_ITERATIONS - iterations, MAX_NUMBER_OF_ITERATIONS);
            fflush(stdout);
        }
        else {
            printf("\rOptimizing the data structure... Iteration %u / %u", MAX_NUMBER_OF_ITERATIONS - iterations, MAX_NUMBER_OF_ITERATIONS);
            fflush(stdout);
        }

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



/* Creating the bounding box hierarchy of a set of objects */

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
        fflush(stdout);
    }
    else {
        printf("\rOptimizing the data structure...");
        fflush(stdout);
    }

    /* Splitting the objects into groups of polygons_per_bounding polygons (on average) */
    const unsigned int k = 1 + content.size() / polygons_per_bounding;
    const std::vector<std::vector<element>> groups = k_means(get_element_vector(content), k);

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
    
    std::vector<element> nodes = get_element_vector(term_nodes);

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

        // printf("nodes.size() = %u\n", nodes.size());
    }

    if (nodes.size() == 1) {
        return nodes[0].bd.value();
    }
    else {
        return containing_bounding_any(get_bounding_vector(nodes));
    }
}


/** Test function **/

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
        unsigned int min = 4294967295;
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
                    min, max, ((double) total) / number_of_nodes);
            }
        }
        else {
            if (number_of_nodes == 1) {
                printf("|| Level %u: nodes: %u, arity: %u\n",
                    level, number_of_nodes, total);
            }
            else {
                printf("|| Level %u: nodes: %u, terminal: %u, minimum arity: %u, maximum: %u, average: %lf\n",
                    level, number_of_nodes, terminal_nodes, min, max, ((double) total) / number_of_nodes);
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