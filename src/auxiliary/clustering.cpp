#include "auxiliary/clustering.hpp"

#include <vector>
#include "scene/objects/object.hpp"
#include "scene/objects/bounding.hpp"

#include<limits>
numeric_limits<double> realclu;
const double infinity = realclu.infinity();

#define MIN_NUMBER_OF_POLYGONS_FOR_BOX 20
#define CARDINAL_OF_BOX_GROUP 2
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

    bool change = false;
    
    for (unsigned int n = 0; n < old_group.size(); n++) {
        for (unsigned int i = 0; i < old_group.at(n).size(); i++) {

            const rt::vector v = old_group.at(n).at(i).get_position();

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
            new_group.at(closest_index).push_back(old_group.at(n).at(i));
        }
    }

    return change;
}

/* Returns a vector of k vectors of elements representing the k clusters */
std::vector<std::vector<element>> k_means(const std::vector<element>& obj, const unsigned int k) {

    /* Vectors containing the k initial means */
    std::vector<rt::vector> means(k);

    /* Filling the vector with k elements uniformly distributed along the obj vector */
    const double step = std::max(obj.size() / k, (unsigned int) 1);

    for (unsigned int i = 0; i < k; i++) {
        means.at(i) = obj.at((int) (i * step)).get_position();
    }
    
    /* I use pointers to vectors to prevent multiple copies of vectors in the while loop below */
    std::vector<std::vector<element>>* group = new std::vector<std::vector<element>>(k);
    assign_to_closest({obj}, *group, means);

    unsigned int iterations = MAX_NUMBER_OF_ITERATIONS;
    bool change = true;
    
    while (change && iterations != 0) {

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

    /* Final vector of groups */
    std::vector<std::vector<element>> output_groups = *group;
    delete(group);

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
    const std::vector<std::vector<element>> groups = k_means(get_element_vector(content), k);

    /** Creating the hierarchy **/

    /* Creating terminal nodes */
    std::vector<const bounding*> term_nodes(k);
    for (unsigned int i = 0; i < k; i++) {
        if (groups.at(i).size() < MIN_NUMBER_OF_POLYGONS_FOR_BOX) {
            term_nodes.at(i) = new bounding(get_object_vector(groups.at(i)));
        }
        else {
            term_nodes.at(i) = containing_objects(get_object_vector(groups.at(i)));
        }
    }

    std::vector<element> nodes = get_element_vector(term_nodes);

    while (nodes.size() > CARDINAL_OF_BOX_GROUP) {
        const std::vector<std::vector<element>> groups = k_means(nodes, CARDINAL_OF_BOX_GROUP);

        std::vector<const bounding*> new_bd_nodes(CARDINAL_OF_BOX_GROUP);
        for (unsigned int i = 0; i < k; i++) {
            new_bd_nodes.at(i) = containing_bounding_any(get_bounding_vector(groups.at(i)));
        }
        nodes.clear();
        nodes = get_element_vector(new_bd_nodes);
    }

    return containing_bounding_any(get_bounding_vector(nodes));    
}
