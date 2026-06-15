#include "file_readers/parsers/obj_parser.hpp"
#include "light/vector.hpp"

#include "scene/objects/bounding.hpp"
#include "scene/objects/triangle.hpp"
#include "scene/objects/quad.hpp"
#include "scene/material/texture.hpp"

#include "accelerating_structures/clustering.hpp"
#include "file_readers/parsers/mtl_parser.hpp"
#include "file_readers/parsers/parsing_wrappers.hpp"
#include "file_readers/file.hpp"

#include <string>
#include <stack>
#include <stdexcept>
#include <filesystem>

static constexpr bool DISPLAY_HIERARCHY = false;
static constexpr bool DO_NOT_APPLY_TEXTURE = false;

/* Quad splitting threshold: when the two triangles forming a quad form an angle
   superior to a certain amount depending on this constant,
   split the quad into two triangles, to solve some visual glitches */
/* The value 1.0E-7 is chosen empirically: it seems to remove all visible glitches by splitting a small number of quads */
/* History: for the stool, 1.0E-6 is sufficient, but leaves visible glitches on the "Porsche 2016" test model. 1.0E-7 removes them. */
static constexpr real QUAD_SPLIT_THRESHOLD = 1.0E-7f;


/* Wavefront .obj file parser */
/* Only handles .obj files made up of triangles and quads, for now.
   In the future, maybe split polygons with >= 5 sides into triangles */

/* When indices are negative, convert them to positive */
static inline void correct(int& v, const int n) {
   v += ((v < 0) ? n + 1 : 0);
}

/* Auxiliary function that adds a trianlge to obj_set and to content if bounded_enabled is true */
static void add_triangle(const std::vector<rt::vector>& vertex_set, const std::vector<rt::vector>& uv_coord_set,
   const std::vector<rt::vector>& normal_set,
   std::vector<const object*>& obj_set, std::vector<const object*>& content, const bool bounding_enabled,
   std::vector<texture_info>& texture_info_set,
   unsigned int& number_of_polygons, unsigned int& number_of_triangles,
   const rt::vector& shift, const real scale,
   const unsigned int v1,  const unsigned int v2,  const unsigned int v3,
   const unsigned int vt1, const unsigned int vt2, const unsigned int vt3,
   const unsigned int vn1, const unsigned int vn2, const unsigned int vn3,
   const unsigned int current_texture_index, const unsigned int current_material_index,
   const bool apply_texture) {

   if (apply_texture) {
      texture_info_set.push_back(
         texture_info(
            current_texture_index,
            std::nullopt, // Temporary: normal maps to be integrated to obj file parsing
            { uv_coord_set[vt1].x,  1 - uv_coord_set[vt1].y,
              uv_coord_set[vt2].x,  1 - uv_coord_set[vt2].y,
              uv_coord_set[vt3].x,  1 - uv_coord_set[vt3].y })
      );
   }

   const triangle* tr =
      new triangle(
         shift + scale * vertex_set[v1],
         shift + scale * vertex_set[v2],
         shift + scale * vertex_set[v3],
         normal_set[vn1], normal_set[vn2], normal_set[vn3],
         current_material_index,
         apply_texture ? texture_info_set.size() - 1 : EMPTY_INDEX);

   obj_set.push_back(tr);

   number_of_polygons ++;
   number_of_triangles ++;

   if (bounding_enabled) {
      content.push_back(tr);
   }
}

/* Auxiliary function that adds a trianlge in a subdivided polygon with more than 5 sides */
static void add_triangle_subdiv(const std::vector<rt::vector>& vertex_set, const std::vector<rt::vector>& uv_coord_set,
   const std::vector<rt::vector>& normal_set,
   std::vector<const object*>& obj_set, std::vector<const object*>& content, const bool bounding_enabled,
   std::vector<texture_info>& texture_info_set,
   unsigned int& number_of_polygons, unsigned int& number_of_triangles,
   const rt::vector& shift, const real scale,
   const unsigned int vj,  const unsigned int vi,  const rt::vector& final_v,
   const unsigned int vtj, const unsigned int vti, const rt::vector& final_vt,
   const unsigned int vnj, const unsigned int vni, const rt::vector& final_vn,
   const unsigned int current_texture_index, const unsigned int current_material_index,
   const bool apply_texture) {

   if (apply_texture) {
      texture_info_set.push_back(
         texture_info(
            current_texture_index,
            std::nullopt, // Temporary: normal maps to be integrated to obj file parsing
            { uv_coord_set[vtj].x, 1 - uv_coord_set[vtj].y,
              uv_coord_set[vti].x, 1 - uv_coord_set[vti].y,
              final_vt.x,          1 - final_vt.y })
      );
   }

   const triangle* tr =
      new triangle(
         shift + scale * vertex_set[vj],
         shift + scale * vertex_set[vi],
         shift + scale * final_v,
         normal_set[vnj], normal_set[vni], final_vn,
         current_material_index, apply_texture ? texture_info_set.size() - 1 : EMPTY_INDEX);

   obj_set.push_back(tr);

   number_of_polygons ++;
   number_of_triangles ++;

   if (bounding_enabled) {
      content.push_back(tr);
   }
}

/* Auxiliary function that adds a quad to obj_set and to content if bounded_enabled is true */
static void add_quad(const std::vector<rt::vector>& vertex_set, const std::vector<rt::vector>& uv_coord_set,
   const std::vector<rt::vector>& normal_set,
   std::vector<const object*>& obj_set, std::vector<const object*>& content, const bool bounding_enabled,
   std::vector<texture_info>& texture_info_set,
   unsigned int& number_of_polygons, unsigned int& number_of_quads,
   const rt::vector& shift, const real scale,
   const unsigned int v1,  const unsigned int v2,  const unsigned int v3,  const unsigned int v4,
   const unsigned int vt1, const unsigned int vt2, const unsigned int vt3, const unsigned int vt4,
   const unsigned int vn1, const unsigned int vn2, const unsigned int vn3, const unsigned int vn4,
   const unsigned int current_texture_index, const unsigned int current_material_index,
   const bool apply_texture) {

   if (apply_texture) {
      texture_info_set.push_back(
         texture_info(
            current_texture_index,
            std::nullopt, // Temporary: normal maps to be integrated to obj file parsing
            { uv_coord_set[vt1].x, 1 - uv_coord_set[vt1].y,
              uv_coord_set[vt2].x, 1 - uv_coord_set[vt2].y,
              uv_coord_set[vt3].x, 1 - uv_coord_set[vt3].y,
              uv_coord_set[vt4].x, 1 - uv_coord_set[vt4].y })
      );
   }

   const quad* q =
      new quad(
         shift + scale * vertex_set[v1],
         shift + scale * vertex_set[v2],
         shift + scale * vertex_set[v3],
         shift + scale * vertex_set[v4],
         normal_set[vn1], normal_set[vn2], normal_set[vn3], normal_set[vn4],
         current_material_index, apply_texture ? texture_info_set.size() - 1 : EMPTY_INDEX);

   obj_set.push_back(q);
               
   number_of_polygons ++;
   number_of_quads ++;

   if (bounding_enabled) {
      content.push_back(q);
   }
}


/* Auxiliary function that adds a trianlge with no vertex normal */
static void add_triangle_no_normal(const std::vector<rt::vector>& vertex_set, const std::vector<rt::vector>& uv_coord_set,
   std::vector<const object*>& obj_set, std::vector<const object*>& content, const bool bounding_enabled,
   std::vector<texture_info>& texture_info_set,
   unsigned int& number_of_polygons, unsigned int& number_of_triangles,
   const rt::vector& shift, const real scale,
   const unsigned int v1,  const unsigned int v2,  const unsigned int v3,
   const unsigned int vt1, const unsigned int vt2, const unsigned int vt3,
   const unsigned int current_texture_index, const unsigned int current_material_index,
   const bool apply_texture) {

   if (apply_texture) {
      texture_info_set.push_back(
         texture_info(
            current_texture_index,
            std::nullopt, // Temporary: normal maps to be integrated to obj file parsing
            { uv_coord_set[vt1].x, 1 - uv_coord_set[vt1].y,
              uv_coord_set[vt2].x, 1 - uv_coord_set[vt2].y,
              uv_coord_set[vt3].x, 1 - uv_coord_set[vt3].y })
      );
   }

   const triangle* tr =
      new triangle(
         shift + scale * vertex_set[v1],
         shift + scale * vertex_set[v2],
         shift + scale * vertex_set[v3],
         current_material_index, apply_texture ? texture_info_set.size() - 1 : EMPTY_INDEX);

   obj_set.push_back(tr);

   number_of_polygons ++;
   number_of_triangles ++;

   if (bounding_enabled) {
      content.push_back(tr);
   }
}

/* Auxiliary function that adds a trianlge in a subdivided polygon with more than 5 sides, with no vertex normal */
static void add_triangle_subdiv_no_normal(const std::vector<rt::vector>& vertex_set, const std::vector<rt::vector>& uv_coord_set,
   std::vector<const object*>& obj_set, std::vector<const object*>& content, const bool bounding_enabled,
   std::vector<texture_info>& texture_info_set,
   unsigned int& number_of_polygons, unsigned int& number_of_triangles,
   const rt::vector& shift, const real scale,
   const unsigned int vj,  const unsigned int vi,  const rt::vector& final_v,
   const unsigned int vtj, const unsigned int vti, const rt::vector& final_vt,
   const unsigned int current_texture_index, const unsigned int current_material_index,
   const bool apply_texture) {

   if (apply_texture) {
      texture_info_set.push_back(
         texture_info(
            current_texture_index,
            std::nullopt, // Temporary: normal maps to be integrated to obj file parsing
            { uv_coord_set[vtj].x, 1 - uv_coord_set[vtj].y,
              uv_coord_set[vti].x, 1 - uv_coord_set[vti].y,
              final_vt.x,          1 - final_vt.y })
      );
   }

   const triangle* tr =
      new triangle(
         shift + scale * vertex_set[vj],
         shift + scale * vertex_set[vi],
         shift + scale * final_v,
         current_material_index, apply_texture ? texture_info_set.size() - 1 : EMPTY_INDEX);

   obj_set.push_back(tr);

   number_of_polygons ++;
   number_of_triangles ++;

   if (bounding_enabled) {
      content.push_back(tr);
   }
}

/* Auxiliary function that adds a quad with no vertex normal */
static void add_quad_no_normal(const std::vector<rt::vector>& vertex_set, const std::vector<rt::vector>& uv_coord_set,
   std::vector<const object*>& obj_set, std::vector<const object*>& content, const bool bounding_enabled,
   std::vector<texture_info>& texture_info_set,
   unsigned int& number_of_polygons, unsigned int& number_of_quads,
   const rt::vector& shift, const real scale,
   const unsigned int v1,  const unsigned int v2,  const unsigned int v3,  const unsigned int v4,
   const unsigned int vt1, const unsigned int vt2, const unsigned int vt3, const unsigned int vt4,
   const unsigned int current_texture_index, const unsigned int current_material_index,
   const bool apply_texture) {

   if (apply_texture) {
      texture_info_set.push_back(
         texture_info(
            current_texture_index,
            std::nullopt, // Temporary: normal maps to be integrated to obj file parsing
            { uv_coord_set[vt1].x, 1 - uv_coord_set[vt1].y,
              uv_coord_set[vt2].x, 1 - uv_coord_set[vt2].y,
              uv_coord_set[vt3].x, 1 - uv_coord_set[vt3].y,
              uv_coord_set[vt4].x, 1 - uv_coord_set[vt4].y })
      );
   }

   const quad* q =
      new quad(
         shift + scale * vertex_set[v1],
         shift + scale * vertex_set[v2],
         shift + scale * vertex_set[v3],
         shift + scale * vertex_set[v4],
         current_material_index, apply_texture ? texture_info_set.size() - 1 : EMPTY_INDEX);

   obj_set.push_back(q);
               
   number_of_polygons ++;
   number_of_quads ++;

   if (bounding_enabled) {
      content.push_back(q);
   }
}

/* Auxiliary function that subdivides a polygon with more than 5 sides into triangles, and adds all of them */
/* Handles syntaxes "f v/vt/vn ..." and "f v//vn ..." */
static void add_subdivided_polygon(const file& f,
   const std::vector<rt::vector>& vertex_set, const std::vector<rt::vector>& uv_coord_set,
   const std::vector<rt::vector>& normal_set,
   std::vector<const object*>& obj_set, std::vector<const object*>& content, const bool bounding_enabled,
   std::vector<texture_info>& texture_info_set,
   unsigned int& number_of_polygons, unsigned int& number_of_triangles,
   const rt::vector& shift, const real scale,
   const unsigned int v1, const unsigned int v2, const unsigned int v3, const unsigned int v4, const unsigned int v5,
   const unsigned int vt1, const unsigned int vt2, const unsigned int vt3, const unsigned int vt4, const unsigned int vt5,
   const unsigned int vn1, const unsigned int vn2, const unsigned int vn3, const unsigned int vn4, const unsigned int vn5,
   const unsigned int current_texture_index, const unsigned int current_material_index,
   bool apply_texture) {
   
   std::stack<unsigned int> v_stack;
   std::stack<unsigned int> vt_stack;
   std::stack<unsigned int> vn_stack;
   for (unsigned int v : { v1, v2, v3, v4, v5 })
      v_stack.push(v);
   for (unsigned int vt : { vt1, vt2, vt3, vt4, vt5 })
      vt_stack.push(vt);
   for (unsigned int vn : { vn1, vn2, vn3, vn4, vn5 })
      v_stack.push(vn);
   
   unsigned int cpt = 5;
   rt::vector final_v  = vertex_set[v1]    + vertex_set[v2]    + vertex_set[v3]    + vertex_set[v4]    + vertex_set[v5];
   rt::vector final_vt = uv_coord_set[vt1] + uv_coord_set[vt2] + uv_coord_set[vt3] + uv_coord_set[vt4] + uv_coord_set[vt5];
   rt::vector final_vn = normal_set[vn1]   + normal_set[vn2]   + normal_set[vn3]   + normal_set[vn4]   + normal_set[vn5];

   // Reading triplets until the end of the line
   char c = f.getc();
   while (c != '\n' && c != EOF) {
      f.ungetc(c);
   
      int vi, vti, vni;
      const int ret = f.scanf_count(" %d/%d/%d", vi, vti, vni);
      if (ret < 1) {
         throw std::runtime_error("Error in parsing of polygons of at least 5 sides (with normal)\n");
      }
      else if (ret == 1) {
         const int ret2 = f.scanf_count("/%d", vni);
         if (ret2 != 1) {
            throw std::runtime_error("Error in parsing of polygons of at least 5 sides (with normal) [2]\n");
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

      c = f.getc();
   }
   f.ungetc(c);

   // New central vertex
   final_v = final_v / cpt;
   if (apply_texture) { final_vt = final_vt / cpt; }
   final_vn = final_vn / cpt;

   // Keeping the last vertex in memory to form a triangle with the first vertex
   const unsigned int last_v  = v_stack.top();
   unsigned int last_vt = 0;
   if (apply_texture) { last_vt = vt_stack.top(); }
   const unsigned int last_vn = vn_stack.top();

   // Adding the new triangles having the new central vertex as a common vertex
   for (unsigned int i = 0; i < cpt - 1; i++) {
      const unsigned int vi = v_stack.top();
      unsigned int vti = 0;
      if (apply_texture) { vti = vt_stack.top(); }
      const unsigned int vni = vn_stack.top();

      v_stack.pop();
      if (apply_texture) { vt_stack.pop(); }
      vn_stack.pop();

      const unsigned int vj = v_stack.top();
      unsigned int vtj = 0;
      if (apply_texture) { vtj = vt_stack.top(); }
      const unsigned int vnj = vn_stack.top();

      add_triangle_subdiv(vertex_set, uv_coord_set, normal_set, obj_set, content, bounding_enabled,
         texture_info_set,
         number_of_polygons, number_of_triangles,
         shift, scale,
         vj, vi, final_v,
         vtj, vti, final_vt,
         vnj, vni, final_vn,
         current_texture_index, current_material_index, apply_texture);
   }
            
   // Adding the last triangle
   add_triangle_subdiv(vertex_set, uv_coord_set, normal_set, obj_set, content, bounding_enabled,
      texture_info_set,
      number_of_polygons, number_of_triangles,
      shift, scale,
      last_v, v1, final_v,
      last_vt, vt1, final_vt,
      last_vn, vn1, final_vn,
      current_texture_index, current_material_index, apply_texture);
}

/* Auxiliary function that subdivides a polygon with more than 5 sides into triangles with no vertex normal, and adds all of them */
/* Handles syntaxes "f v ..." and "f v/vt ..." */
static void add_subdivided_polygon_no_normal(const file& f,
   const std::vector<rt::vector>& vertex_set, const std::vector<rt::vector>& uv_coord_set,
   std::vector<const object*>& obj_set, std::vector<const object*>& content, const bool bounding_enabled,
   std::vector<texture_info>& texture_info_set,
   unsigned int& number_of_polygons, unsigned int& number_of_triangles,
   const rt::vector& shift, const real scale,
   const unsigned int v1,  const unsigned int v2,  const unsigned int v3,  const unsigned int v4,  const unsigned int v5,
   const unsigned int vt1, const unsigned int vt2, const unsigned int vt3, const unsigned int vt4, const unsigned int vt5,
   const unsigned int current_texture_index, const unsigned int current_material_index,
   bool apply_texture) {
   
   std::stack<unsigned int> v_stack;
   std::stack<unsigned int> vt_stack;
   for (unsigned int v : { v1, v2, v3, v4, v5 })
      v_stack.push(v);
   for (unsigned int vt : { vt1, vt2, vt3, vt4, vt5 })
      vt_stack.push(vt);

   unsigned int cpt = 5;
   rt::vector final_v  = vertex_set[v1]    + vertex_set[v2]    + vertex_set[v3]    + vertex_set[v4]    + vertex_set[v5];
   rt::vector final_vt = uv_coord_set[vt1] + uv_coord_set[vt2] + uv_coord_set[vt3] + uv_coord_set[vt4] + uv_coord_set[vt5];

   // Reading triplets until the end of the line
   char c = f.getc();
   while (c != '\n' && c != EOF) {
      f.ungetc(c);
      int vi, vti;
      const int ret = f.scanf_count("%d/%d", vi, vti);

      if (ret < 1) {
         throw std::runtime_error("Error in parsing of polygons of at least 5 sides (without normal)\n");
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

      c = f.getc();
   }
   f.ungetc(c);

   // New central vertex
   final_v = final_v / cpt;
   if (apply_texture) { final_vt = final_vt / cpt; }

   // Keeping the last vertex in memory to form a triangle with the first vertex
   const unsigned int last_v  = v_stack.top();
   unsigned int last_vt = 0;
   if (apply_texture) { last_vt = vt_stack.top(); }

   // Adding the new triangles having the new central vertex as a common vertex
   for (unsigned int i = 0; i < cpt - 1; i++) {
      const unsigned int vi = v_stack.top();
      unsigned int vti = 0;
      if (apply_texture) { vti = vt_stack.top(); }

      v_stack.pop();
      if (apply_texture) { vt_stack.pop(); }

      const unsigned int vj = v_stack.top();
      unsigned int vtj = 0;
      if (apply_texture) { vtj = vt_stack.top(); }

      add_triangle_subdiv_no_normal(vertex_set, uv_coord_set, obj_set, content, bounding_enabled,
         texture_info_set,
         number_of_polygons, number_of_triangles,
         shift, scale,
         vj, vi, final_v,
         vtj, vti, final_vt,
         current_texture_index, current_material_index, apply_texture);
   }
            
   // Adding the last triangle
   add_triangle_subdiv_no_normal(vertex_set, uv_coord_set, obj_set, content, bounding_enabled,
      texture_info_set,
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

exit_status parse_obj_file(const std::string& file_name, const std::optional<unsigned int> default_texture_index,
   std::vector<const object*>& obj_set,
   std::vector<wrapper<material>>& material_wrapper_set,
   std::vector<wrapper<texture>>& texture_wrapper_set,
   std::vector<texture_info>& texture_info_set,
   const real scale, const rt::vector& shift,
   const bool bounding_enabled, const unsigned int polygons_per_bounding,
   const bounding*& output_bd, const real gamma) {

   printf("Parsing obj file...");
   fflush(stdout);

   file f(file_name);

   /* Extraction of the path to the .obj file, to be appended to relative paths of mtl and texture files */
   const std::filesystem::path path = std::filesystem::path(file_name).parent_path();

   /* Storage */
   /* All indices start at 1, so for simplicity we add an unused first vector */
   
   /* Vertices of the object */
   std::vector<rt::vector> vertex_set  (1);

   /* UV-coordinates ("vt"), only the first two attributes (x, y) of the vectors are used */
   std::vector<rt::vector> uv_coord_set(1);

   /* Vertex normals */
   std::vector<rt::vector> normal_set  (1);

   /* Material -> texture association table */
   std::map<unsigned int, unsigned int> mt_assoc;

   unsigned int current_material_index = 0;
   unsigned int current_texture_index = default_texture_index.value_or(EMPTY_INDEX);

   const bool default_texture_provided = default_texture_index.has_value();
   bool apply_texture = default_texture_provided;

   /* Counters */
   unsigned int number_of_vertices        = 0;
   unsigned int number_of_triangles       = 0;
   unsigned int number_of_quads           = 0;
   unsigned int number_of_polygons        = 0;
   unsigned int number_of_texture_coords  = 0;
   unsigned int number_of_normals         = 0;

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

   try {

      // unsigned int cpt_loop = 0;

      /* Parsing loop */
      while (not f.eof()) {

         // cpt_loop ++;
         // if (cpt_loop % 1000 == 0) printf("\r%u vertices %u polygons %u normals %llu", cpt_loop, number_of_vertices, number_of_polygons, normal_set.size());

         // longest items are usemtl, mtllib
         std::array<char, 7> buffer = make_array<char, 7>('\0');
         const exit_status status = f.read<char>({ buffer.data(), 6 });
         if (status == exit_status::Failure) {
            break;
         }
         const std::string s(buffer.data());

         /* New group definition
            If bounding_enabled is true, the current content vector of polygons
            is placed in a box that is added to the children vector */
         if (bounding_enabled && (s == "o" || s == "g") && not content.empty()) {

            // Create a bounding hierarchy containing all the nodes
            /* Heuristic: each group is a depth 1 node in the global bounding box hierarchy */
            const bounding* bd = create_bounding_hierarchy(std::move(content), polygons_per_bounding);
            if constexpr (DISPLAY_HIERARCHY) {
               display_hierarchy_properties(bd);
            }
            children.push_back(bd);
            content.clear();
         }

         /* Commented line, or ignored command */
         if (s == "#" || s == "s" || s == "l" || s == "g" || s == "o" || s == "vp") {

            f.skip_line();
         }
         else if (s == "v") {
            /* Vertex definition */
            const auto [ x, y, z ] = f.scan<double, 3>();
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
         else if (s == "vt") {
            /* Texture UV-coordinates definition */
            const auto [ u, v ] = f.scan<double, 2>();
            
            if (u >= 0 && u <= 1 && v >= 0 && v <= 1) {
               uv_coord_set.push_back(rt::vector(u, v, 0));
            }
            else {
               // Case that happened in one obj file
               const real nu = (u >= 0) ? 1.0_r : ((u <= (-1.0_r)) ? 0.0_r : 1.0_r + u);
               const real nv = (v >= 0) ? 1.0_r : ((v <= (-1.0_r)) ? 0.0_r : 1.0_r + v);
               uv_coord_set.push_back(rt::vector(nu, nv, 0));
            }
            number_of_texture_coords++;
         }
         else if (s == "vn") {
            /* Vertex normal definition */
            const auto [ x, y, z ] = f.scan<double, 3>();
            normal_set.push_back(rt::vector(x, y, z));
            number_of_normals++;
         }
         else if (s == "usemtl") {
            /* Using a new material */
            const std::string m_name = f.read_string(64);

            /* Looking up the material name in the vector of already declared material names */
            const std::optional<unsigned int> vindex = wrapper<material>::find_element(material_wrapper_set, m_name);
            
            if (not vindex.has_value()) {
               throw std::runtime_error("(material reading)");
            }
            
            current_material_index = vindex.value();
            if (mt_assoc.count(current_material_index) > 0) {
               // A texture was associated with the material by an mtl file
               current_texture_index = mt_assoc[current_material_index];
               apply_texture = true;
            }
            else {
               current_texture_index = default_texture_index.value_or(EMPTY_INDEX);
               apply_texture = default_texture_provided;
            }
            
         }
         else if (s == "mtllib") {
            const std::string mtl_file_name = f.read_string(512);

            const exit_status mtl_parsing_successful =
               parse_mtl_file(path, mtl_file_name, material_wrapper_set,
                  texture_wrapper_set, mt_assoc, gamma);

            if (mtl_parsing_successful == exit_status::Failure) {
               throw std::runtime_error("(mtl file loading)");
            }
         }
         else if (s == "f") {
            /* Face declaration */

            /* 3 or 4 vertices will be parsed */
            int v1, vt1, vn1, v2, vt2, vn2, v3, vt3, vn3, v4, vt4, vn4, v5, vt5, vn5;
            const int ret = f.scanf_count("%d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d",
               v1, vt1, vn1, v2, vt2, vn2, v3, vt3, vn3, v4, vt4, vn4, v5, vt5, vn5);

            if (ret >= 9) {
               correct(v1,  number_of_vertices);
               correct(vt1, number_of_texture_coords);
               correct(vn1, number_of_normals);

               correct(v2,  number_of_vertices);
               correct(vt2, number_of_texture_coords);
               correct(vn2, number_of_normals);

               correct(v3,  number_of_vertices);
               correct(vt3, number_of_texture_coords);
               correct(vn3, number_of_normals);
            }
            if (ret >= 12) {
               correct(v4,  number_of_vertices);
               correct(vt4, number_of_texture_coords);
               correct(vn4, number_of_normals);
            }
            if (ret >= 15) {
               correct(v5,  number_of_vertices);
               correct(vt5, number_of_texture_coords);
               correct(vn5, number_of_normals);
            }

            if (ret == 9) {
               /* Triangle */

               add_triangle(vertex_set, uv_coord_set, normal_set, obj_set, content, bounding_enabled,
                  texture_info_set,
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
                     texture_info_set,
                     number_of_polygons, number_of_triangles,
                     shift, scale,
                     v1, v2, v3,
                     vt1, vt2, vt3,
                     vn1, vn2, vn3,
                     current_texture_index, current_material_index, apply_texture);

                  add_triangle(vertex_set, uv_coord_set, normal_set, obj_set, content, bounding_enabled,
                     texture_info_set,
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
                     texture_info_set,
                     number_of_polygons, number_of_quads,
                     shift, scale,
                     v1, v2, v3, v4,
                     vt1, vt2, vt3, vt4,
                     vn1, vn2, vn3, vn4,
                     current_texture_index, current_material_index, apply_texture);
               }
            }
            else if (ret >= 15) {     
               /* Polygons with more than 4 sides */

               add_subdivided_polygon(f, vertex_set, uv_coord_set, normal_set, obj_set, content, bounding_enabled,
                  texture_info_set,
                  number_of_polygons, number_of_triangles,
                  shift, scale,
                  v1, v2, v3, v4, v5,
                  vt1, vt2, vt3, vt4, vt5,
                  vn1, vn2, vn3, vn4, vn5,
                  current_texture_index, current_material_index, apply_texture);
            }
            else if (ret == 1) {
               /* Neither texture coordinates nor the vertex normals are specified */

               const char c = f.getc();
               if (c == ' ') {
                  // f v1 v2 v3 ...

                  const int ret2 = f.scanf_count("%d %d %d %d", v2, v3, v4, v5);

                  correct(v1, number_of_vertices);
                  if (ret2 >= 1) correct(v2, number_of_vertices);
                  if (ret2 >= 2) correct(v3, number_of_vertices);
                  if (ret2 >= 3) correct(v4, number_of_vertices);
                  if (ret2 >= 4) correct(v5, number_of_vertices);

                  if (ret2 == 2) {
                     // Triangle with no texture and normal
                     add_triangle_no_normal(vertex_set, uv_coord_set, obj_set, content, bounding_enabled,
                        texture_info_set,
                        number_of_polygons, number_of_triangles,
                        shift, scale,
                        v1, v2, v3,
                        0, 0, 0,
                        0, current_material_index, DO_NOT_APPLY_TEXTURE);
                  }
                  else if (ret2 == 3) {
                     // Quad with no texture and normal
                     const rt::vector n12 = ((vertex_set[v2] - vertex_set[v1]) ^ (vertex_set[v3] - vertex_set[v1])).unit();
                     const rt::vector n23 = ((vertex_set[v3] - vertex_set[v1]) ^ (vertex_set[v4] - vertex_set[v1])).unit();
                     
                     if ((n12 - n23).normsq() > QUAD_SPLIT_THRESHOLD) {
                        // Splitting into triangles with no texture and normal
                        add_triangle_no_normal(vertex_set, uv_coord_set, obj_set, content, bounding_enabled,
                           texture_info_set,
                           number_of_polygons, number_of_triangles,
                           shift, scale,
                           v1, v2, v3,
                           0, 0, 0,
                           0, current_material_index, DO_NOT_APPLY_TEXTURE);

                        add_triangle_no_normal(vertex_set, uv_coord_set, obj_set, content, bounding_enabled,
                           texture_info_set,
                           number_of_polygons, number_of_triangles,
                           shift, scale,
                           v1, v3, v4,
                           0, 0, 0,
                           0, current_material_index, DO_NOT_APPLY_TEXTURE);
                     }
                     else {
                        // Keeping the quad with no texture and normal
                        add_quad_no_normal(vertex_set, uv_coord_set, obj_set, content, bounding_enabled,
                           texture_info_set,
                           number_of_polygons, number_of_quads,
                           shift, scale,
                           v1, v2, v3, v4,
                           0, 0, 0, 0,
                           0, current_material_index, DO_NOT_APPLY_TEXTURE);
                     }
                  }
                  else {
                     // Untextured polygon with more than 5 sides
                     add_subdivided_polygon_no_normal(f, vertex_set, uv_coord_set, obj_set, content, bounding_enabled,
                        texture_info_set,
                        number_of_polygons, number_of_triangles,
                        shift, scale,
                        v1, v2, v3, v4, v5,
                        0, 0, 0, 0, 0,
                        0, current_material_index, DO_NOT_APPLY_TEXTURE);
                  }
               }
               else {
                  // By elimination: c = '/'
                  // f v1//vn1 v2//vn2 ...
                  const int ret2 = f.scanf_count("%d %d//%d %d//%d %d//%d %d//%d",
                     vn1, v2, vn2, v3, vn3, v4, vn4, v5, vn5);

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
                        texture_info_set,
                        number_of_polygons, number_of_triangles,
                        shift, scale,
                        v1, v2, v3,
                        0, 0, 0,
                        vn1, vn2, vn3,
                        0, current_material_index, DO_NOT_APPLY_TEXTURE);
                  }
                  else if (ret2 == 7) {
                     // Untextured quad
                     const rt::vector n12 = ((vertex_set[v2] - vertex_set[v1]) ^ (vertex_set[v3] - vertex_set[v1])).unit();
                     const rt::vector n23 = ((vertex_set[v3] - vertex_set[v1]) ^ (vertex_set[v4] - vertex_set[v1])).unit();
                     
                     if ((n12 - n23).normsq() > QUAD_SPLIT_THRESHOLD) {
                        // Splitting into two untextured triangles
                        add_triangle(vertex_set, uv_coord_set, normal_set, obj_set, content, bounding_enabled,
                           texture_info_set,
                           number_of_polygons, number_of_triangles,
                           shift, scale,
                           v1, v2, v3,
                           0, 0, 0,
                           vn1, vn2, vn3,
                           0, current_material_index, DO_NOT_APPLY_TEXTURE);

                        add_triangle(vertex_set, uv_coord_set, normal_set, obj_set, content, bounding_enabled,
                           texture_info_set,
                           number_of_polygons, number_of_triangles,
                           shift, scale,
                           v1, v3, v4,
                           0, 0, 0,
                           vn1, vn3, vn4,
                           0, current_material_index, DO_NOT_APPLY_TEXTURE);
                     }
                     else {
                        // Keeping the untextured quad
                        add_quad(vertex_set, uv_coord_set, normal_set, obj_set, content, bounding_enabled,
                           texture_info_set,
                           number_of_polygons, number_of_quads,
                           shift, scale,
                           v1, v2, v3, v4,
                           0, 0, 0, 0,
                           vn1, vn2, vn3, vn4,
                           0, current_material_index, DO_NOT_APPLY_TEXTURE);
                     }
                  }
                  else {
                     // Untextured polygon with more than 5 sides
                     add_subdivided_polygon(f, vertex_set, uv_coord_set, normal_set, obj_set, content, bounding_enabled,
                        texture_info_set,
                        number_of_polygons, number_of_triangles,
                        shift, scale,
                        v1, v2, v3, v4, v5,
                        0, 0, 0, 0, 0,
                        vn1, vn2, vn3, vn4, vn5,
                        0, current_material_index, DO_NOT_APPLY_TEXTURE);
                  }
               }
               
            }
            else {
               // ret == 2
               // f v1/vt1 v2/vt2 ...
               const int ret2 = f.scanf_count("%d/%d %d/%d %d/%d %d/%d",
                  v2, vt2, v3, vt3, v4, vt4, v5, vt5);

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
                     texture_info_set,
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
                        texture_info_set,
                        number_of_polygons, number_of_triangles,
                        shift, scale,
                        v1, v2, v3,
                        vt1, vt2, vt3,
                        current_texture_index, current_material_index, apply_texture);

                     add_triangle_no_normal(vertex_set, uv_coord_set, obj_set, content, bounding_enabled,
                        texture_info_set,
                        number_of_polygons, number_of_triangles,
                        shift, scale,
                        v1, v3, v4,
                        vt1, vt3, vt4,
                        current_texture_index, current_material_index, apply_texture);
                  }
                  else {
                     // Keeping the quad with no normal
                     add_quad_no_normal(vertex_set, uv_coord_set, obj_set, content, bounding_enabled,
                        texture_info_set,
                        number_of_polygons, number_of_quads,
                        shift, scale,
                        v1, v2, v3, v4,
                        vt1, vt2, vt3, vt4,
                        current_texture_index, current_material_index, apply_texture);
                  }
               }
               else {
                  // Polygon with more than 5 sides with no normal
                  add_subdivided_polygon_no_normal(f, vertex_set, uv_coord_set, obj_set, content, bounding_enabled,
                     texture_info_set,
                     number_of_polygons, number_of_triangles,
                     shift, scale,
                     v1, v2, v3, v4, v5,
                     vt1, vt2, vt3, vt4, vt5,
                     current_texture_index, current_material_index, apply_texture);
               }
            }
         }
      }
      
      f.close();
      
      if (bounding_enabled) {
         /* Placing the last group into a bounding */
         // const bounding* bd = containing_objects(content);
         const bounding* bd = create_bounding_hierarchy(std::move(content), polygons_per_bounding);
         if constexpr (DISPLAY_HIERARCHY) {
            display_hierarchy_properties(bd);
         }
         children.push_back(bd);

         /* Setting the final bounding */
         if (children.size() == 1)
            output_bd = children[0];
         else
            output_bd = create_hierarchy_from_boundings(std::move(children));
      }

      printf("\r%s successfully loaded:\n", file_name.c_str());
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

      return exit_status::Success;

   }
   catch(const std::exception& e) {
      printf("Parsing error in file %s ", file_name.c_str());
      printf("%s\n", e.what());
      return exit_status::Failure;
   }
}

