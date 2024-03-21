#include "auxiliary/clustering.hpp"

#include <vector>
#include "scene/objects/object.hpp"

#include<limits>
numeric_limits<double> realclu;
const double infinity = realclu.infinity();


/** K-means clustering algorithm **/


/* Auxiliary function that computes the centroid of a vector of objects */
rt::vector compute_centroid(std::vector<const object*>& obj) {
    double sum_x = 0;
    double sum_y = 0;
    double sum_z = 0;

    /* Computation of the dimensions of the object set */
    for(unsigned int i = 0; i < obj.size(); i++) {
        
        const rt::vector pos = obj.at(i)->get_position();
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
bool assign_to_closest(const std::vector<std::vector<const object*>>& old_group, std::vector<std::vector<const object*>>& new_group,
    const std::vector<rt::vector>& means) {

    bool change = false;
    
    for (unsigned int n = 0; n < old_group.size(); n++) {
        for (unsigned int i = 0; i < old_group.at(n).size(); i++) {

            const rt::vector v = old_group.at(n).at(i)->get_position();

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

/* Returns a vector of k vectors of object* representing the k clusters */
std::vector<std::vector<const object*>> k_means(const std::vector<const object*>& obj, const unsigned int k) {

    /* Vectors containing the k initial means */
    std::vector<rt::vector> means(k);

    /* Filling the vector with k elements uniformly distributed along the obj vector */
    const double step = (obj.size() < k) ? 1 : obj.size() / k;

    for (unsigned int i = 0; i < k; i++) {
        means.at(i) = obj.at((int) (i * step))->get_position();
    }
    
    /* I use pointers to vectors to prevent multiple copies of vectors in the while loop below */
    std::vector<std::vector<const object*>>* group = new std::vector<std::vector<const object*>>(k);
    assign_to_closest({obj}, *group, means);

    unsigned int iterations = 50;
    bool change = true;
    
    while (change && iterations != 0) {

        /* Updating the means */
        for (unsigned int i = 0; i < means.size(); i++) {
            means.at(i) = compute_centroid(group->at(i));
        }

        /* Re-assigning the objects to the right group */
        std::vector<std::vector<const object*>>* new_group = new std::vector<std::vector<const object*>>(k);
        change = assign_to_closest(*group, *new_group, means);
        delete(group);
        group = new_group;

        iterations--;
    }

    /* Final vector of groups */
    std::vector<std::vector<const object*>> output_groups = *group;
    delete(group);

    return output_groups;
}