#include "file_readers/parsers/obj_parser.hpp"

#include "scene/objects/bounding.hpp"
#include "scene/objects/triangle.hpp"
#include "scene/objects/quad.hpp"
#include "scene/material/texture.hpp"

#include "accelerating_structures/clustering.hpp"
#include "file_readers/parsers/mtl_parser.hpp"
#include "file_readers/parsers/parsing_wrappers.hpp"
#include "file_readers/file.hpp"
#include "auxiliary/utils.hpp"

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
static constexpr real QUAD_SPLIT_THRESHOLD = 1.0e-7_r;

static constexpr unsigned int MAX_NAME_LENGTH     = 64;
static constexpr unsigned int MAX_FILENAME_LENGTH = 512;


struct sets {
   const std::vector<rt::vector>&   vertex_set;
   const std::vector<rt::vector>&   uv_coord_set;
   const std::vector<rt::vector>&   normal_set;
   std::vector<const object*>&      obj_set;
   std::vector<const object*>&      content;
   std::vector<texture_info>&       texture_info_set;
};

/* Wavefront .obj file parser */
/* Only handles .obj files made up of triangles and quads, for now.
   In the future, maybe split polygons with >= 5 sides into triangles */

/* When indices are negative, convert them to positive */
static inline void correct(int& v, const int n) {
   v += ((v < 0) ? n + 1 : 0);
}

template<int size = 3>
requires (size >= 2 && size <= 4)
static inline void new_texture_info(std::vector<texture_info>& texture_info_set,
   const std::vector<rt::vector>& uv_coord_set,
   const unsigned int current_texture_index,
   //unsigned int vt1, const unsigned int vt2, const unsigned int vt3) {
   std::array<unsigned int, size>&& vt, const rt::vector& final_vt = rt::vector()) {

   std::array<real, 8> uv {};

   for (int i = 0; unsigned int vti : vt) {
      uv[2 * i]     = uv_coord_set[vti].x;
      uv[2 * i + 1] = 1 - uv_coord_set[vti].y;
      i++;
   }

   if constexpr (size == 2) {
      uv[4] = final_vt.x;
      uv[5] = 1 - final_vt.y;
   }

   texture_info_set.emplace_back(
      current_texture_index,
      std::nullopt, // Temporary: normal maps to be integrated to obj file parsing
      std::move(uv)
   );
}

enum class normal {
   Enabled, Disabled
};

template<int size = 3, normal normal = normal::Enabled>
requires (size >= 2 && size <= 4)
static inline const object* build_polygon(const std::vector<rt::vector>& vertex_set, const rt::vector shift, const real scale,
   const std::vector<rt::vector>& normal_set,
   const unsigned int current_material_index, const bool apply_texture, const unsigned int current_texture_info_set_index,
   std::array<unsigned int, size>&& v, std::array<unsigned int, size>&& vn,
   const rt::vector& final_v   = rt::vector(), const rt::vector& final_vn = rt::vector()
) {

   const unsigned int texture_info_index = apply_texture ? current_texture_info_set_index : EMPTY_INDEX;

   const auto& [ ...vi ]     = v;
   const auto  [ ...vert_i ] = std::array { fma(vertex_set[vi], scale, shift)... };

   const auto& [ ...vni ]    = vn;
   const auto& [ ...norm_i ] = std::array { normal_set[vni]... };

   using polygon = std::conditional_t<size <= 3, triangle, quad>;
   
   if constexpr (normal == normal::Enabled) {
      if constexpr (size >= 3)
         return new polygon(vert_i..., norm_i..., current_material_index, texture_info_index);
      else
         return new polygon(vert_i..., final_v, norm_i..., final_vn, current_material_index, texture_info_index);
   }
   else {
      if constexpr (size >= 3)
         return new polygon(vert_i..., current_material_index, texture_info_index);
      else
         return new polygon(vert_i..., final_v, current_material_index, texture_info_index);
   }
}

template<int size = 3, normal normal = normal::Enabled>
requires (size == 2 || size == 3)
static inline const triangle* build_triangle(const std::vector<rt::vector>& vertex_set, const rt::vector shift, const real scale,
   const std::vector<rt::vector>& normal_set,
   const unsigned int current_material_index, const bool apply_texture, const unsigned int current_texture_info_set_index,
   std::array<unsigned int, size>&& v, std::array<unsigned int, size>&& vn,
   const rt::vector& final_v   = rt::vector(), const rt::vector& final_vn = rt::vector()) {

   const object* obj = build_polygon<size, normal>(vertex_set, shift, scale, normal_set,
      current_material_index, apply_texture, current_texture_info_set_index,
      std::forward<std::array<unsigned int, size>>(v), std::forward<std::array<unsigned int, size>>(vn), final_v, final_vn);
   
   return static_cast<const triangle*>(obj);
}

template <normal normal = normal::Enabled>
static inline const quad* build_quad(const std::vector<rt::vector>& vertex_set, const rt::vector shift, const real scale,
   const std::vector<rt::vector>& normal_set,
   const unsigned int current_material_index, const bool apply_texture, const unsigned int current_texture_info_set_index,
   std::array<unsigned int, 4>&& v, std::array<unsigned int, 4>&& vn) {
   
   const object* obj = build_polygon<4, normal>(vertex_set, shift, scale, normal_set,
      current_material_index, apply_texture, current_texture_info_set_index,
      std::move(v), std::move(vn));

   return static_cast<const quad*>(obj);
}

static inline void push_polygon(
   const object* polygon,
   std::vector<const object*>& obj_set,
   std::vector<const object*>& content,
   unsigned int& number_of_polygons, unsigned int& number_of__type__,
   const bool bounding_enabled) {

   obj_set.push_back(polygon);

   number_of_polygons++;
   number_of__type__++;

   if (bounding_enabled)
      content.push_back(polygon);
}

/* Auxiliary function that adds a trianlge to obj_set and to content if bounded_enabled is true */
static inline void add_triangle(const sets& sets,
   const bool bounding_enabled,
   unsigned int& number_of_polygons, unsigned int& number_of_triangles,
   const rt::vector& shift, const real scale,
   const unsigned int v1,  const unsigned int v2,  const unsigned int v3,
   const unsigned int vt1, const unsigned int vt2, const unsigned int vt3,
   const unsigned int vn1, const unsigned int vn2, const unsigned int vn3,
   const unsigned int current_texture_index, const unsigned int current_material_index,
   const bool apply_texture) {

   auto& [ vertex_set, uv_coord_set, normal_set, obj_set, content, texture_info_set ] = sets;

   if (apply_texture)
      new_texture_info(texture_info_set, uv_coord_set, current_texture_index, { vt1, vt2, vt3 });

   const triangle* tr = build_triangle(vertex_set, shift, scale, normal_set,
      current_material_index, apply_texture, texture_info_set.size() - 1,
      { v1, v2, v3 }, { vn1, vn2, vn3 });

   push_polygon(tr, obj_set, content, number_of_polygons, number_of_triangles, bounding_enabled);
}

/* Auxiliary function that adds a trianlge in a subdivided polygon with more than 5 sides */
static inline void add_triangle_subdiv(const std::vector<rt::vector>& vertex_set, const std::vector<rt::vector>& uv_coord_set,
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

   if (apply_texture)
      new_texture_info<2>(texture_info_set, uv_coord_set, current_texture_index, { vtj, vti }, final_vt);

   const triangle* tr = build_triangle<2>(vertex_set, shift, scale, normal_set,
      current_material_index, apply_texture, texture_info_set.size() - 1,
      { vj, vi }, { vnj, vni }, final_v, final_vn);

   push_polygon(tr, obj_set, content, number_of_polygons, number_of_triangles, bounding_enabled);
}

/* Auxiliary function that adds a quad to obj_set and to content if bounded_enabled is true */
static inline void add_quad(const sets& sets, const bool bounding_enabled,
   unsigned int& number_of_polygons, unsigned int& number_of_quads,
   const rt::vector& shift, const real scale,
   const unsigned int v1,  const unsigned int v2,  const unsigned int v3,  const unsigned int v4,
   const unsigned int vt1, const unsigned int vt2, const unsigned int vt3, const unsigned int vt4,
   const unsigned int vn1, const unsigned int vn2, const unsigned int vn3, const unsigned int vn4,
   const unsigned int current_texture_index, const unsigned int current_material_index,
   const bool apply_texture) {

   auto& [ vertex_set, uv_coord_set, normal_set, obj_set, content, texture_info_set ] = sets;

   if (apply_texture)
      new_texture_info<4>(texture_info_set, uv_coord_set, current_texture_index, { vt1, vt2, vt3, vt4 });

   const quad* q = build_quad(vertex_set, shift, scale, normal_set,
      current_material_index, apply_texture, texture_info_set.size() - 1,
      { v1, v2, v3, v4 }, { vn1, vn2, vn3, vn4 });

   push_polygon(q, obj_set, content, number_of_polygons, number_of_quads, bounding_enabled);
}


/* Auxiliary function that adds a trianlge with no vertex normal */
static inline void add_triangle_no_normal(const sets& sets, const bool bounding_enabled,
   unsigned int& number_of_polygons, unsigned int& number_of_triangles,
   const rt::vector& shift, const real scale,
   const unsigned int v1,  const unsigned int v2,  const unsigned int v3,
   const unsigned int vt1, const unsigned int vt2, const unsigned int vt3,
   const unsigned int current_texture_index, const unsigned int current_material_index,
   const bool apply_texture) {

   auto& [ vertex_set, uv_coord_set, normal_set, obj_set, content, texture_info_set ] = sets;

   if (apply_texture)
      new_texture_info(texture_info_set, uv_coord_set, current_texture_index, { vt1, vt2, vt3 });

   const triangle* tr = build_triangle<3, normal::Disabled>(
      vertex_set, shift, scale, normal_set,
      current_material_index, apply_texture, texture_info_set.size() - 1,
      { v1, v2, v3 }, {});

   push_polygon(tr, obj_set, content, number_of_polygons, number_of_triangles, bounding_enabled);
}

/* Auxiliary function that adds a trianlge in a subdivided polygon with more than 5 sides, with no vertex normal */
static inline void add_triangle_subdiv_no_normal(const sets& sets, const bool bounding_enabled,
   unsigned int& number_of_polygons, unsigned int& number_of_triangles,
   const rt::vector& shift, const real scale,
   const unsigned int vj,  const unsigned int vi,  const rt::vector& final_v,
   const unsigned int vtj, const unsigned int vti, const rt::vector& final_vt,
   const unsigned int current_texture_index, const unsigned int current_material_index,
   const bool apply_texture) {

   auto& [ vertex_set, uv_coord_set, normal_set, obj_set, content, texture_info_set ] = sets;

   if (apply_texture)
      new_texture_info<2>(texture_info_set, uv_coord_set, current_texture_index, { vtj, vti }, final_vt);

   const triangle* tr = build_triangle<2, normal::Disabled>(
      vertex_set, shift, scale, normal_set,
      current_material_index, apply_texture, texture_info_set.size() - 1,
      { vj, vi }, {}, final_v);

   push_polygon(tr, obj_set, content, number_of_polygons, number_of_triangles, bounding_enabled);
}

/* Auxiliary function that adds a quad with no vertex normal */
static inline void add_quad_no_normal(const sets& sets, const bool bounding_enabled,
   unsigned int& number_of_polygons, unsigned int& number_of_quads,
   const rt::vector& shift, const real scale,
   const unsigned int v1,  const unsigned int v2,  const unsigned int v3,  const unsigned int v4,
   const unsigned int vt1, const unsigned int vt2, const unsigned int vt3, const unsigned int vt4,
   const unsigned int current_texture_index, const unsigned int current_material_index,
   const bool apply_texture) {

   auto& [ vertex_set, uv_coord_set, normal_set, obj_set, content, texture_info_set ] = sets;

   if (apply_texture)
      new_texture_info<4>(texture_info_set, uv_coord_set, current_texture_index, { vt1, vt2, vt3, vt4 });

   const quad* q = build_quad<normal::Disabled>(
      vertex_set, shift, scale, normal_set,
      current_material_index, apply_texture, texture_info_set.size() - 1,
      { v1, v2, v3, v4 }, {});

   push_polygon(q, obj_set, content, number_of_polygons, number_of_quads, bounding_enabled);
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
      cpt++;

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
   const sets& sets, const bool bounding_enabled,
   unsigned int& number_of_polygons, unsigned int& number_of_triangles,
   const rt::vector& shift, const real scale,
   const unsigned int v1,  const unsigned int v2,  const unsigned int v3,  const unsigned int v4,  const unsigned int v5,
   const unsigned int vt1, const unsigned int vt2, const unsigned int vt3, const unsigned int vt4, const unsigned int vt5,
   const unsigned int current_texture_index, const unsigned int current_material_index,
   bool apply_texture) {
   
   auto& [ vertex_set, uv_coord_set, normal_set, obj_set, content, texture_info_set ] = sets;

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
      cpt++;

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

      add_triangle_subdiv_no_normal(sets, bounding_enabled,
         number_of_polygons, number_of_triangles,
         shift, scale,
         vj, vi, final_v,
         vtj, vti, final_vt,
         current_texture_index, current_material_index, apply_texture);
   }
            
   // Adding the last triangle
   add_triangle_subdiv_no_normal(sets, bounding_enabled,
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

   printf("Parsing obj file... ");
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

   /* Min-max dimensions */
   rt::vector min = min_max_coord::min_empty;
   rt::vector max = min_max_coord::max_empty;

   /* Bounding containers
      content will contain the polygons of a group before being placed in a bounding,
      which will be added to the children vector
      At the end, a bounding containing all the ones in bounding is placed in output_bounding */
   std::vector<const object*> content;
   std::vector<const bounding*> children;

   sets sets { vertex_set, uv_coord_set, normal_set, obj_set, content, texture_info_set };

   try {

      // unsigned int cpt_loop = 0;

      /* Parsing loop */
      while (not f.eof()) {

         // longest items are usemtl, mtllib
         const std::string arg = f.read_string(6);
         
         /* New group definition
            If bounding_enabled is true, the current content vector of polygons
            is placed in a box that is added to the children vector */
         if (bounding_enabled && (arg == "o" || arg == "g") && not content.empty()) {

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
         if (arg.starts_with("#") || belongs_to(arg, { "s", "l", "g", "o", "vp" })) {

            f.skip_line();
            continue;
         }

         if (arg == "v") {
            /* Vertex definition */
            const auto [ x, y, z ] = f.scan<double, 3>();
            vertex_set.emplace_back(x, y, z);
            number_of_vertices++;

            /* Updating max dimensions */
            min = {
               std::min(min.x, x),
               std::min(min.y, y),
               std::min(min.z, z)
            };
            max = {
               std::max(max.x, x),
               std::max(max.y, y),
               std::max(max.z, z)
            };

            continue;
         }
         
         if (arg == "vt") {
            /* Texture UV-coordinates definition */
            const auto [ u, v ] = f.scan<double, 2>();
            
            if (is_between_zero_and_one(u) && is_between_zero_and_one(v)) [[likely]] {
               uv_coord_set.emplace_back(u, v, 0);
            }
            else {
               // Case that happened in one obj file
               const real nu = (u >= 0) ? 1.0_r : ((u <= (-1.0_r)) ? 0.0_r : 1.0_r + u);
               const real nv = (v >= 0) ? 1.0_r : ((v <= (-1.0_r)) ? 0.0_r : 1.0_r + v);
               uv_coord_set.emplace_back(nu, nv, 0);
            }
            number_of_texture_coords++;

            continue;
         }

         if (arg == "vn") {
            /* Vertex normal definition */
            const auto [ x, y, z ] = f.scan<double, 3>();
            normal_set.emplace_back(x, y, z);
            number_of_normals++;

            continue;
         }

         if (arg == "usemtl") {
            /* Using a new material */
            const std::string m_name = f.read_string(MAX_NAME_LENGTH);

            /* Looking up the material name in the vector of already declared material names */
            const std::optional<unsigned int> vindex = wrapper<material>::find_element(material_wrapper_set, m_name);
            throw_if_null(vindex, "(material reading)");
            
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
            
            continue;
         }
         
         if (arg == "mtllib") {
            const std::string mtl_file_name = f.read_string(MAX_FILENAME_LENGTH);

            const exit_status mtl_parsing_successful =
               parse_mtl_file(path, mtl_file_name, material_wrapper_set,
                  texture_wrapper_set, mt_assoc, gamma);
            throw_if_failure(mtl_parsing_successful, "(mtl file loading)");

            continue;
         }

         if (arg == "f") {
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

               add_triangle(sets, bounding_enabled,
                  number_of_polygons, number_of_triangles,
                  shift, scale,
                  v1,  v2,  v3,
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

                  add_triangle(sets, bounding_enabled,
                     number_of_polygons, number_of_triangles,
                     shift, scale,
                     v1,  v2,  v3,
                     vt1, vt2, vt3,
                     vn1, vn2, vn3,
                     current_texture_index, current_material_index, apply_texture);

                  add_triangle(sets, bounding_enabled,
                     number_of_polygons, number_of_triangles,
                     shift, scale,
                     v1,  v3,  v4,
                     vt1, vt3, vt4,
                     vn1, vn3, vn4,
                     current_texture_index, current_material_index, apply_texture);
               }
               else {
                  /* Coplanar vertices: keeping the quad */
                  
                  add_quad(sets, bounding_enabled,
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
                     add_triangle_no_normal(sets, bounding_enabled,
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
                        add_triangle_no_normal(sets, bounding_enabled,
                           number_of_polygons, number_of_triangles,
                           shift, scale,
                           v1, v2, v3,
                           0, 0, 0,
                           0, current_material_index, DO_NOT_APPLY_TEXTURE);

                        add_triangle_no_normal(sets, bounding_enabled,
                           number_of_polygons, number_of_triangles,
                           shift, scale,
                           v1, v3, v4,
                           0, 0, 0,
                           0, current_material_index, DO_NOT_APPLY_TEXTURE);
                     }
                     else {
                        // Keeping the quad with no texture and normal
                        add_quad_no_normal(sets, bounding_enabled,
                           number_of_polygons, number_of_quads,
                           shift, scale,
                           v1, v2, v3, v4,
                           0, 0, 0, 0,
                           0, current_material_index, DO_NOT_APPLY_TEXTURE);
                     }
                  }
                  else {
                     // Untextured polygon with more than 5 sides
                     add_subdivided_polygon_no_normal(f, sets, bounding_enabled,
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
                     add_triangle(sets, bounding_enabled,
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
                        add_triangle(sets, bounding_enabled,
                           number_of_polygons, number_of_triangles,
                           shift, scale,
                           v1, v2, v3,
                           0, 0, 0,
                           vn1, vn2, vn3,
                           0, current_material_index, DO_NOT_APPLY_TEXTURE);

                        add_triangle(sets, bounding_enabled,
                           number_of_polygons, number_of_triangles,
                           shift, scale,
                           v1, v3, v4,
                           0, 0, 0,
                           vn1, vn3, vn4,
                           0, current_material_index, DO_NOT_APPLY_TEXTURE);
                     }
                     else {
                        // Keeping the untextured quad
                        add_quad(sets, bounding_enabled,
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
                  correct(v1,  number_of_vertices);
                  correct(vt1, number_of_texture_coords);

                  correct(v2,  number_of_vertices);
                  correct(vt2, number_of_texture_coords);

                  correct(v3,  number_of_vertices);
                  correct(vt3, number_of_texture_coords);
               }
               if (ret2 >= 6) {
                  correct(v4,  number_of_vertices);
                  correct(vt4, number_of_texture_coords);
               }
               if (ret2 >= 8) {
                  correct(v5,  number_of_vertices);
                  correct(vt5, number_of_texture_coords);
               }

               if (ret2 == 4) {
                  // Triangle with no normal
                  add_triangle_no_normal(sets, bounding_enabled,
                     number_of_polygons, number_of_triangles,
                     shift, scale,
                     v1,  v2,  v3,
                     vt1, vt2, vt3,
                     current_texture_index, current_material_index, apply_texture);
               }
               else if (ret2 == 6) {
                  // Quad with no normal
                  const rt::vector n12 = ((vertex_set[v2] - vertex_set[v1]) ^ (vertex_set[v3] - vertex_set[v1])).unit();
                  const rt::vector n23 = ((vertex_set[v3] - vertex_set[v1]) ^ (vertex_set[v4] - vertex_set[v1])).unit();
                     
                  if ((n12 - n23).normsq() > QUAD_SPLIT_THRESHOLD) {
                     // Splitting into two triangles with no normal
                     add_triangle_no_normal(sets, bounding_enabled,
                        number_of_polygons, number_of_triangles,
                        shift, scale,
                        v1, v2, v3,
                        vt1, vt2, vt3,
                        current_texture_index, current_material_index, apply_texture);

                     add_triangle_no_normal(sets, bounding_enabled,
                        number_of_polygons, number_of_triangles,
                        shift, scale,
                        v1, v3, v4,
                        vt1, vt3, vt4,
                        current_texture_index, current_material_index, apply_texture);
                  }
                  else {
                     // Keeping the quad with no normal
                     add_quad_no_normal(sets, bounding_enabled,
                        number_of_polygons, number_of_quads,
                        shift, scale,
                        v1, v2, v3, v4,
                        vt1, vt2, vt3, vt4,
                        current_texture_index, current_material_index, apply_texture);
                  }
               }
               else {
                  // Polygon with more than 5 sides with no normal
                  add_subdivided_polygon_no_normal(f, sets, bounding_enabled,
                     number_of_polygons, number_of_triangles,
                     shift, scale,
                     v1, v2, v3, v4, v5,
                     vt1, vt2, vt3, vt4, vt5,
                     current_texture_index, current_material_index, apply_texture);
               }
            }

            continue;
         }
      }
      
      f.close();
      
      if (bounding_enabled) {
         /* Placing the last group into a bounding */
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
         min.x, max.x, min.y, max.y, min.z, max.z);
      if (scale != 1.0_r || not (shift == ZERO)) {
         const rt::vector scaled_min = fma(min, scale, shift);
         const rt::vector scaled_max = fma(max, scale, shift);
         printf("Rescaled/shifted dimensions: (x: [%lf; %lf]; y: [%lf; %lf]; z: [%lf; %lf])\n",
            scaled_min.x, scaled_max.x, scaled_min.y, scaled_max.y, scaled_min.z, scaled_max.z);
      }

      return exit_status::Success;
   }
   catch(const std::exception& e) {
      printf("Parsing error in file %s ", file_name.c_str());
      printf("%s\n", e.what());
      return exit_status::Failure;
   }
}