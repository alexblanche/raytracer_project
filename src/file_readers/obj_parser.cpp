#include "file_readers/obj_parser.hpp"
#include "light/vector.hpp"

#include "scene/objects/bounding.hpp"
#include "scene/objects/triangle.hpp"
#include "scene/objects/quad.hpp"
#include "scene/material/texture.hpp"

#include "auxiliary/clustering.hpp"
#include "file_readers/mtl_parser.hpp"

#include<limits>
numeric_limits<real> realobj; 
const real infinity = realobj.infinity();

#include <string.h>
#include <string>

#include <stack>

#define DISPLAY_HIERARCHY false

#include "file_readers/parsing_wrappers.hpp"
#include <stdexcept>

/* Quad splitting threshold: when the two triangles forming a quad form an angle
   superior to a certain amount depending on this constant,
   split the quad into two triangles, to solve some visual glitches */
/* The value 1.0E-7 is chosen empirically: it seems to remove all visible glitches by splitting a small number of quads */
/* History: for the stool, 1.0E-6 is sufficient, but leaves visible glitches on the "Porsche 2016" test model. 1.0E-7 removes them. */
#define QUAD_SPLIT_THRESHOLD 1.0E-7f


/* Wavefront .obj file parser */
/* Only handles .obj files made up of triangles and quads, for now.
   In the future, maybe split polygons with >= 5 sides into triangles */

/* When indices are negative, convert them to positive */
void correct(int& v, const unsigned int n) {
   if (v < 0)
      v += ((int) n) + 1;
}

/* Auxiliary function that adds a trianlge to obj_set and to content if bounded_enabled is true */
void add_triangle(const std::vector<rt::vector>& vertex_set, const std::vector<rt::vector>& uv_coord_set,
   const std::vector<rt::vector>& normal_set,
   std::vector<const object*>& obj_set, std::vector<const object*>& content, const bool bounding_enabled,
   unsigned int& number_of_polygons, unsigned int& number_of_triangles,
   const rt::vector& shift, const real scale,
   const size_t v1,  const size_t v2,  const size_t v3,
   const size_t vt1, const size_t vt2, const size_t vt3,
   const size_t vn1, const size_t vn2, const size_t vn3,
   const size_t current_texture_index, const size_t current_material_index,
   const bool apply_texture) {

   const std::optional<texture_info> info =
      apply_texture ?
               
      std::optional<texture_info>(texture_info(current_texture_index,
         std::nullopt, // Temporary: normal maps to be integrated to obj file parsing
         {uv_coord_set[vt1].x, 1-uv_coord_set[vt1].y,
         uv_coord_set[vt2].x,  1-uv_coord_set[vt2].y,
         uv_coord_set[vt3].x,  1-uv_coord_set[vt3].y}))
      :
      std::nullopt;

   const triangle* tr =
      new triangle(
         shift + scale * vertex_set[v1],
         shift + scale * vertex_set[v2],
         shift + scale * vertex_set[v3],
         normal_set[vn1], normal_set[vn2], normal_set[vn3],
         current_material_index, info);

   obj_set.push_back(tr);

   number_of_polygons ++;
   number_of_triangles ++;

   if (bounding_enabled) {
      content.push_back(tr);
   }
}

/* Auxiliary function that adds a trianlge in a subdivided polygon with more than 5 sides */
void add_triangle_subdiv(const std::vector<rt::vector>& vertex_set, const std::vector<rt::vector>& uv_coord_set,
   const std::vector<rt::vector>& normal_set,
   std::vector<const object*>& obj_set, std::vector<const object*>& content, const bool bounding_enabled,
   unsigned int& number_of_polygons, unsigned int& number_of_triangles,
   const rt::vector& shift, const real scale,
   const size_t vj,  const size_t vi,  const rt::vector& final_v,
   const size_t vtj, const size_t vti, const rt::vector& final_vt,
   const size_t vnj, const size_t vni, const rt::vector& final_vn,
   const size_t current_texture_index, const size_t current_material_index,
   const bool apply_texture) {

   const std::optional<texture_info> info =
      apply_texture ?

      std::optional<texture_info>(texture_info(current_texture_index,
         std::nullopt, // Temporary: normal maps to be integrated to obj file parsing
         {uv_coord_set[vtj].x, 1-uv_coord_set[vtj].y,
         uv_coord_set[vti].x, 1-uv_coord_set[vti].y,
         final_vt.x, 1-final_vt.y}))
      :
      std::nullopt;

   const triangle* tr =
      new triangle(
         shift + scale * vertex_set[vj],
         shift + scale * vertex_set[vi],
         shift + scale * final_v,
         normal_set[vnj], normal_set[vni], final_vn,
         current_material_index, info);

   obj_set.push_back(tr);

   number_of_polygons ++;
   number_of_triangles ++;

   if (bounding_enabled) {
      content.push_back(tr);
   }
}

/* Auxiliary function that adds a quad to obj_set and to content if bounded_enabled is true */
void add_quad(const std::vector<rt::vector>& vertex_set, const std::vector<rt::vector>& uv_coord_set,
   const std::vector<rt::vector>& normal_set,
   std::vector<const object*>& obj_set, std::vector<const object*>& content, const bool bounding_enabled,
   unsigned int& number_of_polygons, unsigned int& number_of_quads,
   const rt::vector& shift, const real scale,
   const size_t v1,  const size_t v2,  const size_t v3,  const size_t v4,
   const size_t vt1, const size_t vt2, const size_t vt3, const size_t vt4,
   const size_t vn1, const size_t vn2, const size_t vn3, const size_t vn4,
   const size_t current_texture_index, const size_t current_material_index,
   const bool apply_texture) {

   const std::optional<texture_info> info =
      apply_texture ?

      std::optional<texture_info>(texture_info(current_texture_index,
         std::nullopt, // Temporary: normal maps to be integrated to obj file parsing
         {uv_coord_set[vt1].x, 1-uv_coord_set[vt1].y,
         uv_coord_set[vt2].x,  1-uv_coord_set[vt2].y,
         uv_coord_set[vt3].x,  1-uv_coord_set[vt3].y,
         uv_coord_set[vt4].x,  1-uv_coord_set[vt4].y}))
      :
      std::nullopt;

   const quad* q =
      new quad(
         shift + scale * vertex_set[v1],
         shift + scale * vertex_set[v2],
         shift + scale * vertex_set[v3],
         shift + scale * vertex_set[v4],
         normal_set[vn1], normal_set[vn2], normal_set[vn3], normal_set[vn4],
         current_material_index, info);

   obj_set.push_back(q);
               
   number_of_polygons ++;
   number_of_quads ++;

   if (bounding_enabled) {
      content.push_back(q);
   }
}


/* Auxiliary function that adds a trianlge with no vertex normal */
void add_triangle_no_normal(const std::vector<rt::vector>& vertex_set, const std::vector<rt::vector>& uv_coord_set,
   std::vector<const object*>& obj_set, std::vector<const object*>& content, const bool bounding_enabled,
   unsigned int& number_of_polygons, unsigned int& number_of_triangles,
   const rt::vector& shift, const real scale,
   const size_t v1,  const size_t v2,  const size_t v3,
   const size_t vt1, const size_t vt2, const size_t vt3,
   const size_t current_texture_index, const size_t current_material_index,
   const bool apply_texture) {

   const std::optional<texture_info> info =
      apply_texture ?
               
      std::optional<texture_info>(texture_info(current_texture_index,
         std::nullopt, // Temporary: normal maps to be integrated to obj file parsing
         {uv_coord_set[vt1].x, 1-uv_coord_set[vt1].y,
         uv_coord_set[vt2].x,  1-uv_coord_set[vt2].y,
         uv_coord_set[vt3].x,  1-uv_coord_set[vt3].y}))
      :
      std::nullopt;

   const triangle* tr =
      new triangle(
         shift + scale * vertex_set[v1],
         shift + scale * vertex_set[v2],
         shift + scale * vertex_set[v3],
         current_material_index, info);

   obj_set.push_back(tr);

   number_of_polygons ++;
   number_of_triangles ++;

   if (bounding_enabled) {
      content.push_back(tr);
   }
}

/* Auxiliary function that adds a trianlge in a subdivided polygon with more than 5 sides, with no vertex normal */
void add_triangle_subdiv_no_normal(const std::vector<rt::vector>& vertex_set, const std::vector<rt::vector>& uv_coord_set,
   std::vector<const object*>& obj_set, std::vector<const object*>& content, const bool bounding_enabled,
   unsigned int& number_of_polygons, unsigned int& number_of_triangles,
   const rt::vector& shift, const real scale,
   const size_t vj,  const size_t vi,  const rt::vector& final_v,
   const size_t vtj, const size_t vti, const rt::vector& final_vt,
   const size_t current_texture_index, const size_t current_material_index,
   const bool apply_texture) {

   const std::optional<texture_info> info =
      apply_texture ?

      std::optional<texture_info>(texture_info(current_texture_index,
         std::nullopt, // Temporary: normal maps to be integrated to obj file parsing
         {uv_coord_set[vtj].x, 1-uv_coord_set[vtj].y,
         uv_coord_set[vti].x, 1-uv_coord_set[vti].y,
         final_vt.x, 1-final_vt.y}))
      :
      std::nullopt;

   const triangle* tr =
      new triangle(
         shift + scale * vertex_set[vj],
         shift + scale * vertex_set[vi],
         shift + scale * final_v,
         current_material_index, info);

   obj_set.push_back(tr);

   number_of_polygons ++;
   number_of_triangles ++;

   if (bounding_enabled) {
      content.push_back(tr);
   }
}

/* Auxiliary function that adds a quad with no vertex normal */
void add_quad_no_normal(const std::vector<rt::vector>& vertex_set, const std::vector<rt::vector>& uv_coord_set,
   std::vector<const object*>& obj_set, std::vector<const object*>& content, const bool bounding_enabled,
   unsigned int& number_of_polygons, unsigned int& number_of_quads,
   const rt::vector& shift, const real scale,
   const size_t v1,  const size_t v2,  const size_t v3,  const size_t v4,
   const size_t vt1, const size_t vt2, const size_t vt3, const size_t vt4,
   const size_t current_texture_index, const size_t current_material_index,
   const bool apply_texture) {

   const std::optional<texture_info> info =
      apply_texture ?

      std::optional<texture_info>(texture_info(current_texture_index,
         std::nullopt, // Temporary: normal maps to be integrated to obj file parsing
         {uv_coord_set[vt1].x, 1-uv_coord_set[vt1].y,
         uv_coord_set[vt2].x,  1-uv_coord_set[vt2].y,
         uv_coord_set[vt3].x,  1-uv_coord_set[vt3].y,
         uv_coord_set[vt4].x,  1-uv_coord_set[vt4].y}))
      :
      std::nullopt;

   const quad* q =
      new quad(
         shift + scale * vertex_set[v1],
         shift + scale * vertex_set[v2],
         shift + scale * vertex_set[v3],
         shift + scale * vertex_set[v4],
         current_material_index, info);

   obj_set.push_back(q);
               
   number_of_polygons ++;
   number_of_quads ++;

   if (bounding_enabled) {
      content.push_back(q);
   }
}

/* Auxiliary function that subdivides a polygon with more than 5 sides into triangles, and adds all of them */
/* Handles syntaxes "f v/vt/vn ..." and "f v//vn ..." */
void add_subdivided_polygon(FILE* file,
   const std::vector<rt::vector>& vertex_set, const std::vector<rt::vector>& uv_coord_set,
   const std::vector<rt::vector>& normal_set,
   std::vector<const object*>& obj_set, std::vector<const object*>& content, const bool bounding_enabled,
   unsigned int& number_of_polygons, unsigned int& number_of_triangles,
   const rt::vector& shift, const real scale,
   const size_t v1, const size_t v2, const size_t v3, const size_t v4, const size_t v5,
   const size_t vt1, const size_t vt2, const size_t vt3, const size_t vt4, const size_t vt5,
   const size_t vn1, const size_t vn2, const size_t vn3, const size_t vn4, const size_t vn5,
   const size_t current_texture_index, const size_t current_material_index,
   bool apply_texture) {
   
   std::stack<size_t> v_stack;
   std::stack<size_t> vt_stack;
   std::stack<size_t> vn_stack;
   v_stack.push(v1);   v_stack.push(v2);   v_stack.push(v3);   v_stack.push(v4);   v_stack.push(v5);
   vt_stack.push(vt1); vt_stack.push(vt2); vt_stack.push(vt3); vt_stack.push(vt4); vt_stack.push(vt5);
   vn_stack.push(vn1); vn_stack.push(vn2); vn_stack.push(vn3); vn_stack.push(vn4); vn_stack.push(vn5);

   unsigned int cpt = 5;
   rt::vector final_v  = vertex_set[v1]    + vertex_set[v2]    + vertex_set[v3]    + vertex_set[v4]    + vertex_set[v5];
   rt::vector final_vt = uv_coord_set[vt1] + uv_coord_set[vt2] + uv_coord_set[vt3] + uv_coord_set[vt4] + uv_coord_set[vt5];
   rt::vector final_vn = normal_set[vn1]   + normal_set[vn2]   + normal_set[vn3]   + normal_set[vn4]   + normal_set[vn5];

   // Reading triplets until the end of the line
   char c = fgetc(file);
   while (c != '\n' && c != EOF) {
      ungetc(c, file);
      int ret;
      int vi, vti, vni;
      ret = fscanf(file, " %d/%d/%d", &vi, &vti, &vni);
      if (ret < 1) {
         fclose(file);
         printf("Error in parsing of polygons of at least 5 sides (with normal)\n");
         return;
      }
      else if (ret == 1) {
         ret = fscanf(file, "/%d", &vni);
         if (ret != 1) {
            fclose(file);
            printf("Error in parsing of polygons of at least 5 sides (with normal) [2]\n");
            return;
         }
         apply_texture = false;
      }

      correct(vi, vertex_set.size() - 1);
      v_stack.push(vi);
      if (apply_texture) {
         correct(vti, uv_coord_set.size() - 1);
         vt_stack.push(vti);
      }
      correct(vni, normal_set.size() - 1);
      vn_stack.push(vni);

      final_v = final_v + vertex_set[vi];
      if (apply_texture) { final_vt = final_vt + uv_coord_set[vti]; }
      final_vn = final_vn + normal_set[vni];
      cpt ++;

      c = fgetc(file);
   }
   ungetc(c, file);

   // New central vertex
   final_v = final_v / cpt;
   if (apply_texture) { final_vt = final_vt / cpt; }
   final_vn = final_vn / cpt;

   // Keeping the last vertex in memory to form a triangle with the first vertex
   const size_t last_v  = v_stack.top();
   size_t last_vt = 0;
   if (apply_texture) { last_vt = vt_stack.top(); }
   const size_t last_vn = vn_stack.top();

   // Adding the new triangles having the new central vertex as a common vertex
   for (size_t i = 0; i < cpt - 1; i++) {
      const size_t vi = v_stack.top();
      size_t vti = 0;
      if (apply_texture) { vti = vt_stack.top(); }
      const size_t vni = vn_stack.top();

      v_stack.pop();
      if (apply_texture) { vt_stack.pop(); }
      vn_stack.pop();

      const size_t vj = v_stack.top();
      size_t vtj = 0;
      if (apply_texture) { vtj = vt_stack.top(); }
      const size_t vnj = vn_stack.top();

      add_triangle_subdiv(vertex_set, uv_coord_set, normal_set, obj_set, content, bounding_enabled,
         number_of_polygons, number_of_triangles,
         shift, scale,
         vj, vi, final_v,
         vtj, vti, final_vt,
         vnj, vni, final_vn,
         current_texture_index, current_material_index, apply_texture);
   }
            
   // Adding the last triangle
   add_triangle_subdiv(vertex_set, uv_coord_set, normal_set, obj_set, content, bounding_enabled,
      number_of_polygons, number_of_triangles,
      shift, scale,
      last_v, v1, final_v,
      last_vt, vt1, final_vt,
      last_vn, vn1, final_vn,
      current_texture_index, current_material_index, apply_texture);
}

/* Auxiliary function that subdivides a polygon with more than 5 sides into triangles with no vertex normal, and adds all of them */
/* Handles syntaxes "f v ..." and "f v/vt ..." */
void add_subdivided_polygon_no_normal(FILE* file,
   const std::vector<rt::vector>& vertex_set, const std::vector<rt::vector>& uv_coord_set,
   std::vector<const object*>& obj_set, std::vector<const object*>& content, const bool bounding_enabled,
   unsigned int& number_of_polygons, unsigned int& number_of_triangles,
   const rt::vector& shift, const real scale,
   const size_t v1,  const size_t v2,  const size_t v3,  const size_t v4,  const size_t v5,
   const size_t vt1, const size_t vt2, const size_t vt3, const size_t vt4, const size_t vt5,
   const size_t current_texture_index, const size_t current_material_index,
   bool apply_texture) {
   
   std::stack<size_t> v_stack;
   std::stack<size_t> vt_stack;
   v_stack.push(v1);   v_stack.push(v2);   v_stack.push(v3);   v_stack.push(v4);   v_stack.push(v5);
   vt_stack.push(vt1); vt_stack.push(vt2); vt_stack.push(vt3); vt_stack.push(vt4); vt_stack.push(vt5);

   unsigned int cpt = 5;
   rt::vector final_v  = vertex_set[v1]    + vertex_set[v2]    + vertex_set[v3]    + vertex_set[v4]    + vertex_set[v5];
   rt::vector final_vt = uv_coord_set[vt1] + uv_coord_set[vt2] + uv_coord_set[vt3] + uv_coord_set[vt4] + uv_coord_set[vt5];

   // Reading triplets until the end of the line
   char c = fgetc(file);
   while (c != '\n' && c != EOF) {
      ungetc(c, file);
      int ret;
      int vi, vti;
      ret = fscanf(file, "%d/%d", &vi, &vti);

      if (ret < 1) {
         fclose(file);
         printf("Error in parsing of polygons of at least 5 sides (without normal)\n");
         return;
      }
      else if (ret == 1) {
         apply_texture = false;
      }

      correct(vi, vertex_set.size() - 1);
      v_stack.push(vi);
      if (apply_texture) {
         correct(vti, uv_coord_set.size() - 1);
         vt_stack.push(vti);
      }

      final_v = final_v + vertex_set[vi];
      if (apply_texture) { final_vt = final_vt + uv_coord_set[vti]; }
      cpt ++;

      c = fgetc(file);
   }
   ungetc(c, file);

   // New central vertex
   final_v = final_v / cpt;
   if (apply_texture) { final_vt = final_vt / cpt; }

   // Keeping the last vertex in memory to form a triangle with the first vertex
   const size_t last_v  = v_stack.top();
   size_t last_vt = 0;
   if (apply_texture) { last_vt = vt_stack.top(); }

   // Adding the new triangles having the new central vertex as a common vertex
   for (size_t i = 0; i < cpt - 1; i++) {
      const size_t vi = v_stack.top();
      size_t vti = 0;
      if (apply_texture) { vti = vt_stack.top(); }

      v_stack.pop();
      if (apply_texture) { vt_stack.pop(); }

      const size_t vj = v_stack.top();
      size_t vtj = 0;
      if (apply_texture) { vtj = vt_stack.top(); }

      add_triangle_subdiv_no_normal(vertex_set, uv_coord_set, obj_set, content, bounding_enabled,
         number_of_polygons, number_of_triangles,
         shift, scale,
         vj, vi, final_v,
         vtj, vti, final_vt,
         current_texture_index, current_material_index, apply_texture);
   }
            
   // Adding the last triangle
   add_triangle_subdiv_no_normal(vertex_set, uv_coord_set, obj_set, content, bounding_enabled,
      number_of_polygons, number_of_triangles,
      shift, scale,
      last_v, v1, final_v,
      last_vt, vt1, final_vt,
      current_texture_index, current_material_index, apply_texture);
}



/* Parses .obj file file_name. Triangles and quads are added to obj_set,
   with material indices (defined with the keyword usemtl) found in material_names
   
   - Only one texture is handled.
   - Object names (o), polygon groups (g), smooth shading (s), lines (l) are ignored.
   - The object is scaled with the factor scale, and shifted by the vector shift.
   - If bounding_enabled, a bounding containing the whole object is placed in output_bd.
     It contains a hierarchy of bounding boxes, such that the terminal ones contain at most
     polygons_per_bounding polygons.

   Returns true if the operation was successful
*/

bool parse_obj_file(const char* file_name, const std::optional<size_t> default_texture_index,
   std::vector<const object*>& obj_set,
   std::vector<wrapper<material>>& material_wrapper_set,
   std::vector<wrapper<texture>>& texture_wrapper_set,
   const real scale, const rt::vector& shift,
   const bool bounding_enabled, const unsigned int polygons_per_bounding,
   const bounding*& output_bd, const real gamma) {

   printf("Parsing obj file...");
   fflush(stdout);

   FILE* file = fopen(file_name, "r");

   if (file == NULL) {
      printf("Error, .obj file %s not found\n", file_name);
      return false;
   }

   /* Extraction of the path to the .obj file, to be appended to relative paths of mtl and texture files */
   const string file_name_string = string(file_name);
   const size_t last_slash = file_name_string.find_last_of("/\\");
   const string path = file_name_string.substr(0, last_slash + 1);

   /* Storage */
   /* All indices start at 1, so for simplicity we add an unused first vector */
   
   /* Vertices of the object */
   std::vector<rt::vector> vertex_set = { rt::vector() };

   /* UV-coordinates ("vt"), only the first two attributes (x, y) of the vectors are used */
   std::vector<rt::vector> uv_coord_set = { rt::vector() };

   /* Vertex normals */
   std::vector<rt::vector> normal_set = { rt::vector() };

   /* Material -> texture association table */
   map<size_t, size_t> mt_assoc;

   size_t current_material_index = 0;
   size_t current_texture_index = default_texture_index.value_or((size_t) (-1));

   const bool default_texture_provided = default_texture_index.has_value();
   bool apply_texture = default_texture_provided;

   /* Counters */
   unsigned int number_of_vertices = 0;
   unsigned int number_of_triangles = 0;
   unsigned int number_of_quads = 0;
   unsigned int number_of_polygons = 0;
   unsigned int number_of_texture_coords = 0;
   unsigned int number_of_normals = 0;

   /* Max dimensions */
   real min_x = infinity,
      min_y = infinity,
      min_z = infinity,
      max_x = -infinity,
      max_y = -infinity,
      max_z = -infinity;

   /* Bounding containers
      content will contain the polygons of a group before being placed in a bounding,
      which will be added to the children vector
      At the end, a bounding containing all the ones in bounding is placed in output_bounding */
   std::vector<const object*> content;
   std::vector<const bounding*> children;

   // printf("\n");

   try {

      // unsigned int cpt_loop = 0;

      /* Parsing loop */
      while (not feof(file)) {

         // cpt_loop ++;
         // if (cpt_loop % 1000 == 0) printf("\r%u vertices %u polygons %u normals %llu", cpt_loop, number_of_vertices, number_of_polygons, normal_set.size());

         // longest items are usemtl, mtllib
         char s[7];
         if (fscanf(file, "%6s ", s) != 1) {
            break;
         }

         /* New group definition
            If bounding_enabled is true, the current content vector of polygons
            is placed in a box that is added to the children vector */
         if (bounding_enabled && (strcmp(s, "o") == 0 || strcmp(s, "g") == 0) && not content.empty()) {

            // Create a bounding hierarchy containing all the nodes
            /* Heuristic: each group is a depth 1 node in the global bounding box hierarchy */
            const bounding* bd = create_bounding_hierarchy(content, polygons_per_bounding);
            if (DISPLAY_HIERARCHY) {
               display_hierarchy_properties(bd);
            }
            children.push_back(bd);
            content.clear();
         }

         /* Commented line, or ignored command */
         if (strcmp(s, "#") == 0
            || strcmp(s, "s") == 0
            || strcmp(s, "l") == 0
            || strcmp(s, "g") == 0
            || strcmp(s, "o") == 0
            || strcmp(s, "vp") == 0) {

            char c;
            do {
               c = fgetc(file);
            }
            while (c != '\n' && c != EOF);
            ungetc(c, file);
         }
         else if (strcmp(s, "v") == 0) {
            /* Vertex definition */
            double x, y, z;
            const int ret = fscanf(file, "%lf %lf %lf\n", &x, &y, &z);
            if (ret != 3) {
               throw std::runtime_error("(vertex definition)");
            }
            else {
               vertex_set.push_back(rt::vector(x, y, z));
               number_of_vertices ++;

               /* Updating max dimensions */
               if (x > max_x) { max_x = x; }
               if (x < min_x) { min_x = x; }
               if (y > max_y) { max_y = y; }
               if (y < min_y) { min_y = y; }
               if (z > max_z) { max_z = z; }
               if (z < min_z) { min_z = z; }
            }
         }
         else if (strcmp(s, "vt") == 0) {
            /* Texture UV-coordinates definition */
            double u, v;
            const int ret = fscanf(file, "%lf %lf\n", &u, &v);
            if (ret != 2) {
               throw std::runtime_error("(UV-coordinates definition)");
            }
            else {
               if (u >= 0 && u <= 1 && v >= 0 && v <= 1) {
                  uv_coord_set.push_back(rt::vector(u, v, 0));
                  number_of_texture_coords++;
               }
               else {
                  // Case that happened in one obj file
                  const real nu = (u >= 0) ? 1 : ((u <= (-1)) ? 0 : 1 + u);
                  const real nv = (v >= 0) ? 1 : ((v <= (-1)) ? 0 : 1 + v);
                  uv_coord_set.push_back(rt::vector(nu, nv, 0));
                  number_of_texture_coords++;
               }
            }
         }
         else if (strcmp(s, "vn") == 0) {
            /* Vertex normal definition */
            double x, y, z;
            const int ret = fscanf(file, "%lf %lf %lf\n", &x, &y, &z);
            if (ret != 3) {
               throw std::runtime_error("(normal definition)");
            }
            else {
               normal_set.push_back(rt::vector(x, y, z));
               number_of_normals++;
            }
         }
         else if (strcmp(s, "usemtl") == 0) {
            /* Using a new material */
            char m_name[65];
            const int ret = fscanf(file, " %64s", m_name);
            if (ret != 1) {
               throw std::runtime_error("(texture name in usemtl)");
            }

            /* Looking up the material name in the vector of already declared material names */
            std::optional<size_t> vindex = std::nullopt;
            for (wrapper<material> const& mat_wrap : material_wrapper_set) {
               if (mat_wrap.name.has_value() && mat_wrap.name.value().compare(m_name) == 0) {
                  vindex = mat_wrap.index;
               }
            }
            
            if (not vindex.has_value()) {
               printf("Error, material %s not found\n", m_name);
               throw std::runtime_error("(material reading)");
            }
            else {
               current_material_index = vindex.value();
               if (mt_assoc.count(current_material_index) > 0) {
                  // A texture was associated with the material by an mtl file
                  current_texture_index = mt_assoc[current_material_index];
                  apply_texture = true;
               }
               else {
                  current_texture_index = default_texture_index.value_or((size_t) (-1));
                  apply_texture = default_texture_provided;
               }
            }
         }
         else if (strcmp(s, "mtllib") == 0) {
            char mtl_file_name[513];
            const int ret = fscanf(file, " %512s\n", mtl_file_name);
            if (ret != 1) {
               throw std::runtime_error("(mtllib)");
            }

            const bool mtl_parsing_successful =
               parse_mtl_file(mtl_file_name, path, material_wrapper_set,
                  texture_wrapper_set, mt_assoc, gamma);

            if (not mtl_parsing_successful) {
               throw std::runtime_error("(mtl file loading)");
            }
         }
         else if (strcmp(s, "f") == 0) {
            /* Face declaration */

            /* 3 or 4 vertices will be parsed */
            int v1, vt1, vn1, v2, vt2, vn2, v3, vt3, vn3, v4, vt4, vn4, v5, vt5, vn5;
            const int ret = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d",
               &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3, &v4, &vt4, &vn4, &v5, &vt5, &vn5);

            if (ret >= 9) {
               correct(v1, number_of_vertices);
               correct(vt1, number_of_texture_coords);
               correct(vn1, number_of_normals);

               correct(v2, number_of_vertices);
               correct(vt2, number_of_texture_coords);
               correct(vn2, number_of_normals);

               correct(v3, number_of_vertices);
               correct(vt3, number_of_texture_coords);
               correct(vn3, number_of_normals);
            }
            if (ret >= 12) {
               correct(v4, number_of_vertices);
               correct(vt4, number_of_texture_coords);
               correct(vn4, number_of_normals);
            }
            if (ret >= 15) {
               correct(v5, number_of_vertices);
               correct(vt5, number_of_texture_coords);
               correct(vn5, number_of_normals);
            }


            if (ret == 9) {
               /* Triangle */

               add_triangle(vertex_set, uv_coord_set, normal_set, obj_set, content, bounding_enabled,
                  number_of_polygons, number_of_triangles,
                  shift, scale,
                  v1, v2, v3,
                  vt1, vt2, vt3,
                  vn1, vn2, vn3,
                  current_texture_index, current_material_index, apply_texture);
               
            }
            else if (ret == 12) {
               /* Quad */

               /* Sometimes quads are made up of 4 non-coplanar vertices
                  When it is the case, we split the quad in two triangles */

               const rt::vector n12 = ((vertex_set[v2] - vertex_set[v1]) ^ (vertex_set[v3] - vertex_set[v1])).unit();
               const rt::vector n23 = ((vertex_set[v3] - vertex_set[v1]) ^ (vertex_set[v4] - vertex_set[v1])).unit();
                              
               if ((n12 - n23).normsq() > QUAD_SPLIT_THRESHOLD) {
                  /* Non-coplanar vertices: splitting the quad into two triangles */

                  add_triangle(vertex_set, uv_coord_set, normal_set, obj_set, content, bounding_enabled,
                     number_of_polygons, number_of_triangles,
                     shift, scale,
                     v1, v2, v3,
                     vt1, vt2, vt3,
                     vn1, vn2, vn3,
                     current_texture_index, current_material_index, apply_texture);

                  add_triangle(vertex_set, uv_coord_set, normal_set, obj_set, content, bounding_enabled,
                     number_of_polygons, number_of_triangles,
                     shift, scale,
                     v1, v3, v4,
                     vt1, vt3, vt4,
                     vn1, vn3, vn4,
                     current_texture_index, current_material_index, apply_texture);
               }
               else {
                  /* Coplanar vertices: keeping the quad */
                  
                  add_quad(vertex_set, uv_coord_set, normal_set, obj_set, content, bounding_enabled,
                     number_of_polygons, number_of_quads,
                     shift, scale,
                     v1, v2, v3, v4,
                     vt1, vt2, vt3, vt4,
                     vn1, vn2, vn3, vn4,
                     current_texture_index, current_material_index, apply_texture);
               }
            }
            else if (ret > 12) {     
               /* Polygons with more than 4 sides */

               add_subdivided_polygon(file, vertex_set, uv_coord_set, normal_set, obj_set, content, bounding_enabled,
                  number_of_polygons, number_of_triangles,
                  shift, scale,
                  v1, v2, v3, v4, v5,
                  vt1, vt2, vt3, vt4, vt5,
                  vn1, vn2, vn3, vn4, vn5,
                  current_texture_index, current_material_index, apply_texture);
            }
            else if (ret == 1) {
               /* Neither texture coordinates nor the vertex normals are specified */

               const char c = fgetc(file);
               if (c == ' ') {
                  // f v1 v2 v3 ...

                  const int ret2 = fscanf(file, "%d %d %d %d",
                     &v2, &v3, &v4, &v5);

                  correct(v1, number_of_vertices);
                  if (ret2 >= 1) correct(v2, number_of_vertices);
                  if (ret2 >= 2) correct(v3, number_of_vertices);
                  if (ret2 >= 3) correct(v4, number_of_vertices);
                  if (ret2 >= 4) correct(v5, number_of_vertices);

                  if (ret2 == 2) {
                     // Triangle with no texture and normal
                     add_triangle_no_normal(vertex_set, uv_coord_set, obj_set, content, bounding_enabled,
                        number_of_polygons, number_of_triangles,
                        shift, scale,
                        v1, v2, v3,
                        0, 0, 0,
                        0, current_material_index, false);
                  }
                  else if (ret2 == 3) {
                     // Quad with no texture and normal
                     const rt::vector n12 = ((vertex_set[v2] - vertex_set[v1]) ^ (vertex_set[v3] - vertex_set[v1])).unit();
                     const rt::vector n23 = ((vertex_set[v3] - vertex_set[v1]) ^ (vertex_set[v4] - vertex_set[v1])).unit();
                     
                     if ((n12 - n23).normsq() > QUAD_SPLIT_THRESHOLD) {
                        // Splitting into triangles with no texture and normal
                        add_triangle_no_normal(vertex_set, uv_coord_set, obj_set, content, bounding_enabled,
                           number_of_polygons, number_of_triangles,
                           shift, scale,
                           v1, v2, v3,
                           0, 0, 0,
                           0, current_material_index, false);

                        add_triangle_no_normal(vertex_set, uv_coord_set, obj_set, content, bounding_enabled,
                           number_of_polygons, number_of_triangles,
                           shift, scale,
                           v1, v3, v4,
                           0, 0, 0,
                           0, current_material_index, false);
                     }
                     else {
                        // Keeping the quad with no texture and normal
                        add_quad_no_normal(vertex_set, uv_coord_set, obj_set, content, bounding_enabled,
                           number_of_polygons, number_of_quads,
                           shift, scale,
                           v1, v2, v3, v4,
                           0, 0, 0, 0,
                           0, current_material_index, false);
                     }
                  }
                  else {
                     // Untextured polygon with more than 5 sides
                     add_subdivided_polygon_no_normal(file, vertex_set, uv_coord_set, obj_set, content, bounding_enabled,
                        number_of_polygons, number_of_triangles,
                        shift, scale,
                        v1, v2, v3, v4, v5,
                        0, 0, 0, 0, 0,
                        0, current_material_index, false);
                  }
               }
               else {
                  // By elimination: c = '/'
                  // f v1//vn1 v2//vn2 ...
                  const int ret2 = fscanf(file, "%d %d//%d %d//%d %d//%d %d//%d",
                     &vn1, &v2, &vn2, &v3, &vn3, &v4, &vn4, &v5, &vn5);

                  if (ret2 >= 5) {
                     correct(v1, number_of_vertices);
                     correct(vn1, number_of_normals);

                     correct(v2, number_of_vertices);
                     correct(vn2, number_of_normals);

                     correct(v3, number_of_vertices);
                     correct(vn3, number_of_normals);
                  }
                  if (ret2 >= 7) {
                     correct(v4, number_of_vertices);
                     correct(vn4, number_of_normals);
                  }
                  if (ret2 >= 9) {
                     correct(v5, number_of_vertices);
                     correct(vn5, number_of_normals);
                  }

                  if (ret2 == 5) {
                     // Untextured triangle
                     add_triangle(vertex_set, uv_coord_set, normal_set, obj_set, content, bounding_enabled,
                        number_of_polygons, number_of_triangles,
                        shift, scale,
                        v1, v2, v3,
                        0, 0, 0,
                        vn1, vn2, vn3,
                        0, current_material_index, false);
                  }
                  else if (ret2 == 7) {
                     // Untextured quad
                     const rt::vector n12 = ((vertex_set[v2] - vertex_set[v1]) ^ (vertex_set[v3] - vertex_set[v1])).unit();
                     const rt::vector n23 = ((vertex_set[v3] - vertex_set[v1]) ^ (vertex_set[v4] - vertex_set[v1])).unit();
                     
                     if ((n12 - n23).normsq() > QUAD_SPLIT_THRESHOLD) {
                        // Splitting into two untextured triangles
                        add_triangle(vertex_set, uv_coord_set, normal_set, obj_set, content, bounding_enabled,
                           number_of_polygons, number_of_triangles,
                           shift, scale,
                           v1, v2, v3,
                           0, 0, 0,
                           vn1, vn2, vn3,
                           0, current_material_index, false);

                        add_triangle(vertex_set, uv_coord_set, normal_set, obj_set, content, bounding_enabled,
                           number_of_polygons, number_of_triangles,
                           shift, scale,
                           v1, v3, v4,
                           0, 0, 0,
                           vn1, vn3, vn4,
                           0, current_material_index, false);
                     }
                     else {
                        // Keeping the untextured quad
                        add_quad(vertex_set, uv_coord_set, normal_set, obj_set, content, bounding_enabled,
                           number_of_polygons, number_of_quads,
                           shift, scale,
                           v1, v2, v3, v4,
                           0, 0, 0, 0,
                           vn1, vn2, vn3, vn4,
                           0, current_material_index, false);
                     }
                  }
                  else {
                     // Untextured polygon with more than 5 sides
                     add_subdivided_polygon(file, vertex_set, uv_coord_set, normal_set, obj_set, content, bounding_enabled,
                        number_of_polygons, number_of_triangles,
                        shift, scale,
                        v1, v2, v3, v4, v5,
                        0, 0, 0, 0, 0,
                        vn1, vn2, vn3, vn4, vn5,
                        0, current_material_index, false);
                  }
               }
               
            }
            else {
               // ret == 2
               // f v1/vt1 v2/vt2 ...
               const int ret2 = fscanf(file, "%d/%d %d/%d %d/%d %d/%d",
                  &v2, &vt2, &v3, &vt3, &v4, &vt4, &v5, &vt5);

               if (ret2 >= 4) {
                  correct(v1, number_of_vertices);
                  correct(vt1, number_of_texture_coords);

                  correct(v2, number_of_vertices);
                  correct(vt2, number_of_texture_coords);

                  correct(v3, number_of_vertices);
                  correct(vt3, number_of_texture_coords);
               }
               if (ret2 >= 6) {
                  correct(v4, number_of_vertices);
                  correct(vt4, number_of_texture_coords);
               }
               if (ret2 >= 8) {
                  correct(v5, number_of_vertices);
                  correct(vt5, number_of_texture_coords);
               }

               if (ret2 == 4) {
                  // Triangle with no normal
                  add_triangle_no_normal(vertex_set, uv_coord_set, obj_set, content, bounding_enabled,
                     number_of_polygons, number_of_triangles,
                     shift, scale,
                     v1, v2, v3,
                     vt1, vt2, vt3,
                     current_texture_index, current_material_index, apply_texture);
               }
               else if (ret2 == 6) {
                  // Quad with no normal
                  const rt::vector n12 = ((vertex_set[v2] - vertex_set[v1]) ^ (vertex_set[v3] - vertex_set[v1])).unit();
                  const rt::vector n23 = ((vertex_set[v3] - vertex_set[v1]) ^ (vertex_set[v4] - vertex_set[v1])).unit();
                     
                  if ((n12 - n23).normsq() > QUAD_SPLIT_THRESHOLD) {
                     // Splitting into two triangles with no normal
                     add_triangle_no_normal(vertex_set, uv_coord_set, obj_set, content, bounding_enabled,
                        number_of_polygons, number_of_triangles,
                        shift, scale,
                        v1, v2, v3,
                        vt1, vt2, vt3,
                        current_texture_index, current_material_index, apply_texture);

                     add_triangle_no_normal(vertex_set, uv_coord_set, obj_set, content, bounding_enabled,
                        number_of_polygons, number_of_triangles,
                        shift, scale,
                        v1, v3, v4,
                        vt1, vt3, vt4,
                        current_texture_index, current_material_index, apply_texture);
                  }
                  else {
                     // Keeping the quad with no normal
                     add_quad_no_normal(vertex_set, uv_coord_set, obj_set, content, bounding_enabled,
                        number_of_polygons, number_of_quads,
                        shift, scale,
                        v1, v2, v3, v4,
                        vt1, vt2, vt3, vt4,
                        current_texture_index, current_material_index, apply_texture);
                  }
               }
               else {
                  // Polygon with more than 5 sides with no normal
                  add_subdivided_polygon_no_normal(file, vertex_set, uv_coord_set, obj_set, content, bounding_enabled,
                     number_of_polygons, number_of_triangles,
                     shift, scale,
                     v1, v2, v3, v4, v5,
                     vt1, vt2, vt3, vt4, vt5,
                     current_texture_index, current_material_index, apply_texture);
               }
            }
         }
      }
      
      fclose(file);
      
      if (bounding_enabled) {
         /* Placing the last group into a bounding */
         // const bounding* bd = containing_objects(content);
         const bounding* bd = create_bounding_hierarchy(content, polygons_per_bounding);
         if (DISPLAY_HIERARCHY) {
            display_hierarchy_properties(bd);
         }
         children.push_back(bd);

         /* Setting the final bounding */
         if (children.size() == 1) {
            output_bd = children[0];
         }
         else {
            output_bd = create_hierarchy_from_boundings(children);
         }
      }

      printf("\r%s successfully loaded:\n", file_name);
      printf("%u vertices, %u polygons (%u triangles, %u quads)\n",
         number_of_vertices, number_of_polygons, number_of_triangles, number_of_quads);
      printf("Dimensions: (x: [%lf; %lf]; y: [%lf; %lf]; z: [%lf; %lf])\n",
         min_x, max_x, min_y, max_y, min_z, max_z);
      if (scale != 1 || not (shift == rt::vector(0,0,0))) {
         printf("Rescaled/shifted dimensions: (x: [%lf; %lf]; y: [%lf; %lf]; z: [%lf; %lf])\n",
            shift.x + scale * min_x, shift.x + scale * max_x,
            shift.y + scale * min_y, shift.y + scale * max_y,
            shift.z + scale * min_z, shift.z + scale * max_z);   
      }
      fflush(stdout);

      return true;

   }
   catch(const std::exception& e) {
      printf("Parsing error in file %s ", file_name);
      printf("%s\n", e.what());
      fclose(file);
      return false;
   }
}

