#pragma once

#include "light/vector.hpp"
#include "scene/objects/object.hpp"
#include "scene/objects/bounding.hpp"

#include <variant>
#include <vector>

/* K-means clustering algorithm */

template<typename T>
concept ElementContent = std::is_same_v<T, const object*> || std::is_same_v<T, const bounding*>;

/* Struct containing either an object* or a bounding*, to make the clustering functions polymorphic */
struct element {

   enum class type {
      Object, Bounding
   };
   type type;
   std::variant<const object*, const bounding*> content;

   element(const object* o) :
      type(type::Object), content(std::in_place_type_t<const object*>(), o) {}

   element(const bounding* b) :
      type(type::Bounding), content(std::in_place_type_t<const bounding*>(), b) {}

   inline const object* get_object() const {
      return std::get<const object*>(content);
   }
   inline const bounding* get_bounding() const {
      return std::get<const bounding*>(content);
   }

   template<ElementContent T>
   inline T get_content() const {
      return std::get<T>(content);
   }

   inline const rt::vector& get_position() const {
      using enum type;
      switch (type) {
         case Object:
            return std::get<const object*>(content)->get_position();
         
         case Bounding: {
            const bounding* bd = std::get<const bounding*>(content);
            const std::optional<const box*>& b = bd->get_b();
            return (b.has_value()) ?
                 b.value()->get_position()
               : bd->get_content()[0]->get_position();
         }
      }
   }

   template<ElementContent T>
   static std::vector<element> get_element(const std::vector<T>& v) {
      std::vector<element> elts;
      elts.reserve(v.size());
      for (T x : v)
         elts.emplace_back(x);
      return elts;
   }

   template<ElementContent T>
   static std::vector<T> get_content(const std::vector<element>& elts) {
      std::vector<T> v;
      v.reserve(elts.size());
      for (element const& elt : elts)
         v.push_back(elt.get_content<T>());
      return v;
}
};

/* Auxiliary function to create_bounding_hierarchy
   Performs the second step of the algorithm: creates the hierarchy of the terminal boundings */
const bounding* create_hierarchy_from_boundings(std::vector<const bounding*>&& term_nodes);

/* Returns a bounding* containing the objects of content, split into a hierarchy of boundings if their number
   exceeds MIN_NUMBER_OF_POLYGONS_FOR_BOX */
const bounding* create_bounding_hierarchy(std::vector<const object*>&& content,
   const unsigned int polygons_per_bounding);


/** Tests **/

/* Displays the depth of the hierarchy, as well as the minimum, maximum and average arity of each depth */
void display_hierarchy_properties(const bounding* bd);