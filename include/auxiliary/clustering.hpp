#pragma once

#include <vector>
#include "light/vector.hpp"
#include "scene/objects/object.hpp"
#include "scene/objects/bounding.hpp"

/* K-means clustering algorithm */

/* Struct containing either an object* or a bounding*, to make the clustering functions polymorphic */
struct element {
   const object* obj;
   const bounding* bd;

   /* Constructors */

   element() {
      obj = NULL;
      bd = NULL;
   }

   element(const object* o) {
      obj = o;
      bd = NULL;
   }

   element(const bounding* b) {
      obj = NULL;
      bd = b;
   }

   /* Position accessor */

   inline rt::vector get_position() const {
      if (obj != NULL) {
         return obj->get_position();
      }
      else if (bd != NULL && bd->get_b() != NULL) {
         return bd->get_b()->get_position();
      }
      else {
         return bd->get_content().at(0)->get_position();
      }
   }
};

/* Returns a vector of k vectors of elements representing the k clusters */
std::vector<std::vector<element>> k_means(const std::vector<element>& elts, const unsigned int k);

/* Returns a bounding* containing the objects of content, split into a hierarchy of boundings if their number
   exceeds MIN_NUMBER_OF_POLYGONS_FOR_BOX */
const bounding* create_bounding_hierarchy(const std::vector<const object*>& content,
   const unsigned int polygons_per_bounding);


/** Tests **/

/* Displays the depth of the hierarchy, as well as the minimum, maximum and average arity of each depth */
void display_hierarchy_properties(const bounding* bd);