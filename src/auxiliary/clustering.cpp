#include "auxiliary/clustering.hpp"

#include <vector>
#include "scene/objects/object.hpp"
#include "scene/objects/bounding.hpp"

#include <stack>

#include<limits>
numeric_limits<double> realclu;
const double infinity = realclu.infinity();

#define MIN_NUMBER_OF_POLYGONS_FOR_BOX 20
#define CARDINAL_OF_BOX_GROUP 3
#define MAX_NUMBER_OF_ITERATIONS 50

/** K-means clustering algorithm **/

/* Auxiliary function that computes the centroid of a vector of objects */
rt::vector compute_centroid(const std::vector<element>& obj) {
    double sum_x = 0;
    double sum_y = 0;
    double sum_z = 0;

    /* Computation of the dimensions of the object set */
    for(unsigned int i = 0; i < obj.size(); i++) {
        
        const rt::vector pos = obj.at(i).get_position();
        sum_x += pos.x;
        sum_y += pos.y;
        sum_z += pos.z;
    }

    const unsigned int k = obj.size();

    return rt::vector(sum_x / k, sum_y / k, sum_z / k);
}

/* Auxiliary function that adds each element of old_group to one of the k vectors of new_group,
   according to which of the vectors of means they are the closest
   Returns true if there has been a change of group for at least one element */
bool assign_to_closest(const std::vector<std::vector<element>>& old_group, std::vector<std::vector<element>>& new_group,
    const std::vector<rt::vector>& means) {

    // printf("assign_to_closest : begin\n");

    bool change = false;
    
    for (unsigned int n = 0; n < old_group.size(); n++) {
        // printf("old_group.at(n).size() = %u, means.size() = %u\n", old_group.at(n).size(), means.size());
        for (unsigned int i = 0; i < old_group.at(n).size(); i++) {
            // printf("(%u, %u) ", n, i);

            const element elt = old_group.at(n).at(i);
            const rt::vector v = elt.get_position();

            unsigned int closest_index = 0;
            double distance_to_closest = (means.at(0) - v).normsq();

            for (unsigned int m = 1; m < means.size(); m++) {
                const double d = (means.at(m) - v).normsq();
                if(d < distance_to_closest) {
                    distance_to_closest = d;
                    closest_index = m;
                }
            }

            if (closest_index != n) {
                change = true;
            }
            new_group.at(closest_index).push_back(elt);
        }
    }

    // printf("assign_to_closest : end\n");

    return change;
}

/* Returns a vector of k vectors of elements representing the k clusters */
std::vector<std::vector<element>> k_means(const std::vector<element>& obj, const unsigned int k) {

    /* Vectors containing the k initial means */
    std::vector<rt::vector> means(k);

    // printf("k_means: starting\n");

    /* Filling the vector with k elements uniformly distributed along the obj vector */
    const double step = std::max((double) (obj.size() / k), 1.0);

    for (unsigned int i = 0; i < std::min((unsigned int) obj.size(), k); i++) {
        means.at(i) = obj.at((int) (i * step)).get_position();
    }

    // printf("\nk_means: initial means created\n");
    
    /* I use pointers to vectors to prevent multiple copies of vectors in the while loop below */
    std::vector<std::vector<element>>* group = new std::vector<std::vector<element>>(k);
    assign_to_closest({obj}, *group, means);

    // printf("k_means: assign_to_closest done\n");

    unsigned int iterations = MAX_NUMBER_OF_ITERATIONS;
    bool change = true;
    
    while (change && iterations != 0) {

        // printf("k_means: iteration\n");

        /* Updating the means */
        for (unsigned int i = 0; i < means.size(); i++) {
            means.at(i) = compute_centroid(group->at(i));
        }

        /* Re-assigning the objects to the right group */
        std::vector<std::vector<element>>* new_group = new std::vector<std::vector<element>>(k);
        change = assign_to_closest(*group, *new_group, means);
        delete(group);
        group = new_group;

        iterations--;
    }

    printf("k_means: %u iterations (maximum = %u)\n", MAX_NUMBER_OF_ITERATIONS - iterations, MAX_NUMBER_OF_ITERATIONS);
    // printf("k_means: loop exitted\n");

    /* Final vector of groups */
    std::vector<std::vector<element>> output_groups = *group;
    delete(group);

    // printf("k_means: ready to exit\n");

    return output_groups;
}

/** Auxiliary conversion functions **/

std::vector<element> get_element_vector(const std::vector<const object*>& obj) {
    std::vector<element> elts(obj.size());
    for (unsigned int i = 0; i < obj.size(); i++) {
        elts.at(i) = element(obj.at(i));
    }
    return elts;
}

std::vector<element> get_element_vector(const std::vector<const bounding*>& obj) {
    std::vector<element> elts(obj.size());
    for (unsigned int i = 0; i < obj.size(); i++) {
        elts.at(i) = element(obj.at(i));
    }
    return elts;
}

std::vector<const object*> get_object_vector(const std::vector<element>& elts) {
    std::vector<const object*> obj(elts.size());
    for (unsigned int i = 0; i < elts.size(); i++) {
        obj.at(i) = elts.at(i).obj;
    }
    return obj;
}

std::vector<const bounding*> get_bounding_vector(const std::vector<element>& elts) {
    std::vector<const bounding*> bds(elts.size());
    for (unsigned int i = 0; i < elts.size(); i++) {
        bds.at(i) = elts.at(i).bd;
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
   
    /* Splitting the objects into groups of polygons_per_bounding polygons (on average) */
    const unsigned int k = 1 + content.size() / polygons_per_bounding;

    // printf("1 Calling k_means: %u elements, k = %u\n", content.size(), k);
    const std::vector<std::vector<element>> groups = k_means(get_element_vector(content), k);

    /** Creating the hierarchy **/

    // unsigned int cpt = 0;

    /* Creating terminal nodes */
    std::vector<const bounding*> term_nodes;
    for (unsigned int i = 0; i < k; i++) {
        if (groups.at(i).size() != 0) {
            if (groups.at(i).size() < MIN_NUMBER_OF_POLYGONS_FOR_BOX) {
                term_nodes.push_back(new bounding(get_object_vector(groups.at(i))));
            }
            else {
                term_nodes.push_back(containing_objects(get_object_vector(groups.at(i))));
            }
            // cpt ++;
        }
    }
    // printf("1 non-empty nodes: %u, empty nodes: %u\n", cpt, k - cpt);

    std::vector<element> nodes = get_element_vector(term_nodes);
    // printf("nodes.size() = %u\n", nodes.size());

    while (nodes.size() > CARDINAL_OF_BOX_GROUP) {

        const unsigned int k = 1 + nodes.size() / CARDINAL_OF_BOX_GROUP;

        // printf("2 Calling k_means: %u elements, k = %u\n", nodes.size(), k);
        const std::vector<std::vector<element>> groups = k_means(nodes, k);

        std::vector<const bounding*> new_bd_nodes;
        
        unsigned int cpt = 0;
        for (unsigned int i = 0; i < k; i++) {
            
            if (groups.at(i).size() != 0) {
                new_bd_nodes.push_back(containing_bounding_any(get_bounding_vector(groups.at(i))));
                cpt ++;
            }
        }
        printf("non-empty nodes: %u, empty nodes: %u\n", cpt, k - cpt);

        nodes.clear();
        nodes = get_element_vector(new_bd_nodes);

        // printf("nodes.size() = %u\n", nodes.size());
    }

    if (nodes.size() == 1) {
        return nodes.at(0).bd;
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
    printf("Level 0: arity = %u\n", (unsigned int) bd0->get_children().size());

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
            if (bd->is_terminal_bd()) {
                terminal_nodes ++;
            }
            else {
                unsigned int arity = bd->get_children().size();
                if (arity > max) {max = arity;}
                if (arity < min) {min = arity;}
                total += arity;
                for (unsigned int j = 0; j < arity; j++) {
                    next_bds.push(bd->get_children().at(j));
                }
            }
        }

        /* Displaying the statistics */
        if (terminal_nodes == number_of_nodes) {
            printf("Level %u: nodes: %u, all terminal\n", level, number_of_nodes);
        }
        else {
            printf("Level %u: nodes: %u, terminal: %u, minimum arity: %u, maximum: %u, average: %lf\n",
                level, number_of_nodes, terminal_nodes, min, max, ((double) total) / number_of_nodes);
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