#pragma once

#include <vector>
#include "light/vector.hpp"
#include "scene/objects/object.hpp"
#include "scene/objects/bounding.hpp"

/* K-means clustering algorithm */

/* Struct containing either an object* or a bounding*, to make the clustering functions polymorphic */
struct element {
   std::optional<const object*> obj;
   std::optional<const bounding*> bd;

   /* Constructors */

   element() {
      obj = std::nullopt;
      bd = std::nullopt;
   }

   element(const object* o) {
      obj = o;
      bd = std::nullopt;
   }

   element(const bounding* b) {
      obj = std::nullopt;
      bd = b;
   }

   /* Position accessor */

   inline rt::vector get_position() const {
      if (obj.has_value()) {
         return obj.value()->get_position();
      }
      else if (bd.value()->get_b().has_value()) {
         return bd.value()->get_b().value()->get_position();
      }
      else {
         return bd.value()->get_content()[0]->get_position();
      }
   }
};

/* Returns a vector of k vectors of elements representing the k clusters */
std::vector<std::vector<element>> k_means(const std::vector<element>& elts, const unsigned int k);

/* Auxiliary function to create_bounding_hierarchy
   Performs the second step of the algorithm: creates the hierarchy of the terminal boundings */
const bounding* create_hierarchy_from_boundings(const std::vector<const bounding*>& term_nodes);

/* Returns a bounding* containing the objects of content, split into a hierarchy of boundings if their number
   exceeds MIN_NUMBER_OF_POLYGONS_FOR_BOX */
const bounding* create_bounding_hierarchy(const std::vector<const object*>& content,
   const unsigned int polygons_per_bounding);


/** Tests **/

/* Displays the depth of the hierarchy, as well as the minimum, maximum and average arity of each depth */
void display_hierarchy_properties(const bounding* bd);