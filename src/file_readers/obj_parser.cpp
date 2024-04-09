#include "file_readers/obj_parser.hpp"
#include "light/vector.hpp"

#include "scene/objects/bounding.hpp"
#include "scene/objects/triangle.hpp"
#include "scene/objects/quad.hpp"
#include "scene/material/texture.hpp"

#include "auxiliary/clustering.hpp"
#include "file_readers/mtl_parser.hpp"

#include <stdio.h>
#include <string.h>
#include <string>

#include <stack>

#define DISPLAY_HIERARCHY false

#include "scene/objects/sphere.hpp"


/* Wavefront .obj file parser */
/* Only handles .obj files made up of triangles and quads, for now.
   In the future, maybe split polygons with >= 5 sides into triangles */

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

bool parse_obj_file(const char* file_name, const unsigned int default_texture_index,
   std::vector<const object*>& obj_set,
   std::vector<string>& material_names, std::vector<material>& material_set,
   std::vector<string>& texture_names, std::vector<texture>& texture_set,
   const double& scale, const rt::vector& shift,
   const bool bounding_enabled, const unsigned int polygons_per_bounding,
   const bounding*& output_bd) {

   printf("Parsing obj file...");
   fflush(stdout);

   FILE* file = fopen(file_name, "r");

   if (file == NULL) {
      printf("Error, .obj file %s not found\n", file_name);
      return false;
   }

   /* Extraction of the path to the .obj file, to be appended to relative paths of mtl and texture files */
   const string file_name_string = string(file_name);
   const unsigned int last_slash = file_name_string.find_last_of("/\\");
   const string path = file_name_string.substr(0, last_slash + 1);

   /* Storage */
   /* All indices start at 1, so for simplicity we add an unused first vector */
   
   /* Vertices of the object */
   std::vector<rt::vector> vertex_set = {rt::vector()};

   /* UV-coordinates ("vt"), only the first two attributes (x, y) of the vectors are used */
   std::vector<rt::vector> uv_coord_set = {rt::vector()};

   /* Vertex normals */
   std::vector<rt::vector> normal_set = {rt::vector()};

   /* Material -> texture association table */
   mt_assoc assoc = mt_assoc();

   unsigned int current_material_index = -1;
   unsigned int current_texture_index = default_texture_index;

   const bool default_texture_provided = default_texture_index != (unsigned int) -1;
   bool apply_texture = default_texture_provided;

   /* Counters */
   unsigned int number_of_vertices = 0;
   unsigned int number_of_triangles = 0;
   unsigned int number_of_quads = 0;
   unsigned int number_of_polygons = 0;

   /* Bounding containers
      content will contain the polygons of a group before being placed in a bounding,
      which will be added to the children vector
      At the end, a bounding containing all the ones in bounding is placed in output_bounding */
   std::vector<const object*> content;
   std::vector<const bounding*> children;

   /* Parsing loop */
   while (not feof(file)) {

      // longest items are usemtl, mtllib
      char s[7];
      if (fscanf(file, "%6s ", s) != 1) {
         break;
      }

      /* New group definition
         If bounding_enabled is true, the current content vector of polygons
         is placed in a box that is added to the children vector */
      if (bounding_enabled && (strcmp(s, "o") == 0 || strcmp(s, "g") == 0) && content.size() != 0) {

         // First method: put the whole group in one bounding
         // const bounding* bd = containing_objects(content);

         // Second method: create a bounding hierarchy containing all the nodes
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
            fclose(file);
            printf("Parsing error in file %s (vertex definition)\n", file_name);
            return false;
         }
         else {
            vertex_set.push_back(rt::vector(x, y, z));
            number_of_vertices ++;
         }
      }
      else if (strcmp(s, "vt") == 0) {
         /* Texture UV-coordinates definition */
         double u, v;
         const int ret = fscanf(file, "%lf %lf\n", &u, &v);
         if (ret != 2) {
            fclose(file);
            printf("Parsing error in file %s (UV-coordinates definition)\n", file_name);
            return false;
         }
         else {
            if (u >= 0 && u <= 1 && v >= 0 && v <= 1) {
               uv_coord_set.push_back(rt::vector(u, v, 0));
            }
            else {
               // Stupid case in some garbage obj files...
               const double nu = (u >= 0) ? 1 : ((u <= (-1)) ? 0 : 1 + u);
               const double nv = (v >= 0) ? 1 : ((v <= (-1)) ? 0 : 1 + v);
               uv_coord_set.push_back(rt::vector(nu, nv, 0));
            }
         }
      }
      else if (strcmp(s, "vn") == 0) {
         /* Vertex normal definition */
         double x, y, z;
         const int ret = fscanf(file, "%lf %lf %lf\n", &x, &y, &z);
         if (ret != 3) {
            fclose(file);
            printf("Parsing error in file %s (normal definition)\n", file_name);
            return false;
         }
         else {
            normal_set.push_back(rt::vector(x, y, z));
         }
      }
      else if (strcmp(s, "usemtl") == 0) {
         /* Using a new material */
         char m_name[65];
         const int ret = fscanf(file, " %64s", m_name);
         if (ret != 1) {
            fclose(file);
            printf("Parsing error in file %s (texture name in usemtl)\n", file_name);
            return false;
         }

         /* Looking up the material name in the vector of already declared material names */
         unsigned int vindex = -1;
         for (unsigned int i = 0; i < material_names.size(); i++) {
            if (material_names.at(i).compare(m_name) == 0) {
               vindex = i;
               break;
            }
         }
         
         if (vindex == ((unsigned int) -1)) {
            fclose(file);
            printf("Error, material %s not found\n", m_name);
            return false;
         }
         else {
            current_material_index = vindex;
            unsigned int t_index;
            if (assoc.lookup(current_material_index, t_index)) {
               // A texture was associated with the material by an mtl file
               current_texture_index = t_index;
               apply_texture = true;
            }
            else {
               current_texture_index = default_texture_index;
               apply_texture = default_texture_provided;
            }
         }
      }
      else if (strcmp(s, "mtllib") == 0) {
         char mtl_file_name[513];
         const int ret = fscanf(file, " %512s\n", mtl_file_name);
         if (ret != 1) {
            fclose(file);
            printf("Parsing error in file %s (mtllib)\n", file_name);
            return false;
         }

         const bool mtl_parsing_successful =
            parse_mtl_file(mtl_file_name, path, material_names, material_set,
               texture_names, texture_set, assoc);

         if (not mtl_parsing_successful) {
            fclose(file);
            printf("Parsing error in obj file parsing (mtl file loading)\n");
            return false;
         }
      }
      else if (strcmp(s, "f") == 0) {
         /* Face declaration */

         /* 3 or 4 vertices will be parsed */
         unsigned int v1, vt1, vn1, v2, vt2, vn2, v3, vt3, vn3, v4, vt4, vn4, v5, vt5, vn5;
         const int ret = fscanf(file, "%u/%u/%u %u/%u/%u %u/%u/%u %u/%u/%u %u/%u/%u",
            &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3, &v4, &vt4, &vn4, &v5, &vt5, &vn5);

         if (ret == 9) {

            /* Triangle */
            const texture_info info =
               apply_texture ?
               
               texture_info(current_texture_index,
                  {uv_coord_set.at(vt1).x, 1-uv_coord_set.at(vt1).y,
                  uv_coord_set.at(vt2).x, 1-uv_coord_set.at(vt2).y,
                  uv_coord_set.at(vt3).x, 1-uv_coord_set.at(vt3).y})
               :
               texture_info();

            const triangle* tr =
               apply_texture ?

               new triangle(
                  shift + scale * vertex_set.at(v1),
                  shift + scale * vertex_set.at(v2),
                  shift + scale * vertex_set.at(v3),
                  normal_set.at(vn1), normal_set.at(vn2), normal_set.at(vn3),
                  current_material_index, info)
               :
               new triangle(
                  shift + scale * vertex_set.at(v1),
                  shift + scale * vertex_set.at(v2),
                  shift + scale * vertex_set.at(v3),
                  normal_set.at(vn1), normal_set.at(vn2), normal_set.at(vn3),
                  current_material_index);

            obj_set.push_back(tr);

            number_of_polygons ++;
            number_of_triangles ++;

            if (bounding_enabled) {
               content.push_back(tr);
            }
         }
         else if (ret == 12) {

            /* Quad */

            /* Sometimes quads are made up of 4 non-coplanar vertices
               When it is the case, we split the quad in two triangles */

            const rt::vector n12 = ((vertex_set.at(v2) - vertex_set.at(v1)) ^ (vertex_set.at(v3) - vertex_set.at(v1))).unit();
            const rt::vector n23 = ((vertex_set.at(v3) - vertex_set.at(v1)) ^ (vertex_set.at(v4) - vertex_set.at(v1))).unit();
            /* The value 0.001 (squared) is chosen empirically: it seems to remove all visible glitches by splitting a small number of quads */
            if ((n12 - n23).normsq() > 0.000001) {
               /* Non-coplanar vertices: splitting the quad into two triangles */

               const texture_info info12 =
                  apply_texture ?
                  
                  texture_info(current_texture_index,
                     {uv_coord_set.at(vt1).x, 1-uv_coord_set.at(vt1).y,
                     uv_coord_set.at(vt2).x, 1-uv_coord_set.at(vt2).y,
                     uv_coord_set.at(vt3).x, 1-uv_coord_set.at(vt3).y})
                  :
                  texture_info();

               const triangle* tr1 =
                  apply_texture ?

                  new triangle(
                     shift + scale * vertex_set.at(v1),
                     shift + scale * vertex_set.at(v2),
                     shift + scale * vertex_set.at(v3),
                     normal_set.at(vn1), normal_set.at(vn2), normal_set.at(vn3),
                     current_material_index, info12)
                  :
                  new triangle(
                     shift + scale * vertex_set.at(v1),
                     shift + scale * vertex_set.at(v2),
                     shift + scale * vertex_set.at(v3),
                     normal_set.at(vn1), normal_set.at(vn2), normal_set.at(vn3),
                     current_material_index);

               obj_set.push_back(tr1);

               const texture_info info23 =
                  apply_texture ?

                  texture_info(current_texture_index,
                     {uv_coord_set.at(vt1).x, 1-uv_coord_set.at(vt1).y,
                     uv_coord_set.at(vt3).x, 1-uv_coord_set.at(vt3).y,
                     uv_coord_set.at(vt4).x, 1-uv_coord_set.at(vt4).y})
                  :
                  texture_info();

               const triangle* tr2 =
                  apply_texture?
                  
                  new triangle(
                     shift + scale * vertex_set.at(v1),
                     shift + scale * vertex_set.at(v3),
                     shift + scale * vertex_set.at(v4),
                     normal_set.at(vn1), normal_set.at(vn3), normal_set.at(vn4),
                     current_material_index, info23)
                  :
                  new triangle(
                     shift + scale * vertex_set.at(v1),
                     shift + scale * vertex_set.at(v3),
                     shift + scale * vertex_set.at(v4),
                     normal_set.at(vn1), normal_set.at(vn3), normal_set.at(vn4),
                     current_material_index);

               obj_set.push_back(tr2);

               number_of_polygons += 2;
               number_of_triangles += 2;

               if (bounding_enabled) {
                  content.push_back(tr1);
                  content.push_back(tr2);
               }
            }
            else {
               
               /* Coplanar vertices: keeping the quad */
               const texture_info info =
                  apply_texture ?

                  texture_info(current_texture_index,
                     {uv_coord_set.at(vt1).x, 1-uv_coord_set.at(vt1).y,
                     uv_coord_set.at(vt2).x, 1-uv_coord_set.at(vt2).y,
                     uv_coord_set.at(vt3).x, 1-uv_coord_set.at(vt3).y,
                     uv_coord_set.at(vt4).x, 1-uv_coord_set.at(vt4).y})
                  :
                  texture_info();

               const quad* q =
                  apply_texture ?
                  
                  new quad(
                     shift + scale * vertex_set.at(v1),
                     shift + scale * vertex_set.at(v2),
                     shift + scale * vertex_set.at(v3),
                     shift + scale * vertex_set.at(v4),
                     normal_set.at(vn1), normal_set.at(vn2), normal_set.at(vn3), normal_set.at(vn4),
                     current_material_index, info)
                  :
                  new quad(
                     shift + scale * vertex_set.at(v1),
                     shift + scale * vertex_set.at(v2),
                     shift + scale * vertex_set.at(v3),
                     shift + scale * vertex_set.at(v4),
                     normal_set.at(vn1), normal_set.at(vn2), normal_set.at(vn3), normal_set.at(vn4),
                     current_material_index);

               obj_set.push_back(q);
               
               number_of_polygons ++;
               number_of_quads ++;

               if (bounding_enabled) {
                  content.push_back(q);
               }
            }
         }
         else {

            // Temporary workaround (skip the polygons of more than 4 sides)
            // char ca;
            // do {
            //    ca = fgetc(file);
            // }
            // while (ca != '\n' && ca != EOF);
            // ungetc(ca, file);

            // continue;

            
            // Polygons with more than 4 sides
            std::stack<unsigned int> v_stack;
            std::stack<unsigned int> vt_stack;
            std::stack<unsigned int> vn_stack;
            v_stack.push(v1);   v_stack.push(v2);   v_stack.push(v3);   v_stack.push(v4);   v_stack.push(v5);
            vt_stack.push(vt1); vt_stack.push(vt2); vt_stack.push(vt3); vt_stack.push(vt4); vt_stack.push(vt5);
            vn_stack.push(vn1); vn_stack.push(vn2); vn_stack.push(vn3); vn_stack.push(vn4); vn_stack.push(vn5);

            unsigned int cpt = 5;
            rt::vector final_v = vertex_set.at(v1) + vertex_set.at(v2) + vertex_set.at(v3) + vertex_set.at(v4) + vertex_set.at(v5);
            rt::vector final_vt = uv_coord_set.at(vt1) + uv_coord_set.at(vt2) + uv_coord_set.at(vt3) + uv_coord_set.at(vt4) + uv_coord_set.at(vt5);
            rt::vector final_vn = normal_set.at(vn1) + normal_set.at(vn2) + normal_set.at(vn3) + normal_set.at(vn4) + normal_set.at(vn5);
            
            // Reading triplets until the end of the line
            char c = fgetc(file);
            while (c != '\n' && c != EOF) {
               ungetc(c, file);
               unsigned int vi, vti, vni;
               int ret = fscanf(file, " %u/%u/%u", &vi, &vti, &vni);
               if (ret != 3) {
                  fclose(file);
                  printf("Error in parsing of polygons of at least 5 sides\n");
                  return false;
               }

               v_stack.push(vi);
               vt_stack.push(vti);
               vn_stack.push(vni);

               final_v = final_v + vertex_set.at(vi);
               final_vt = final_vt + uv_coord_set.at(vti);
               final_vn = final_vn + normal_set.at(vni);
               cpt ++;

               c = fgetc(file);
            }
            ungetc(c, file);

            // New central vertex
            final_v = final_v / cpt;
            final_vt = final_vt / cpt;
            final_vn = final_vn / cpt;

            // Keeping the last vertex in memory to form a triangle with the first vertex
            const unsigned int last_v = v_stack.top();
            const unsigned int last_vt = vt_stack.top();
            const unsigned int last_vn = vn_stack.top();

            // Adding the new triangles having the new central vertex as a common vertex
            for (unsigned int i = 0; i < cpt - 1; i++) {
               const unsigned int vi = v_stack.top();
               const unsigned int vti = vt_stack.top();
               const unsigned int vni = vn_stack.top();
               v_stack.pop();
               vt_stack.pop();
               vn_stack.pop();
               const unsigned int vj = v_stack.top();
               const unsigned int vtj = vt_stack.top();
               const unsigned int vnj = vn_stack.top();

               const texture_info info =
                  apply_texture ?

                  texture_info(current_texture_index,
                     {uv_coord_set.at(vtj).x, 1-uv_coord_set.at(vtj).y,
                     uv_coord_set.at(vti).x, 1-uv_coord_set.at(vti).y,
                     final_vt.x, 1-final_vt.y})
                  :
                  texture_info();

               const triangle* tr =
                  apply_texture ?

                  new triangle(
                     shift + scale * vertex_set.at(vj),
                     shift + scale * vertex_set.at(vi),
                     shift + scale * final_v,
                     normal_set.at(vnj), normal_set.at(vni), final_vn,
                     current_material_index, info)
                  :
                  new triangle(
                     shift + scale * vertex_set.at(vj),
                     shift + scale * vertex_set.at(vi),
                     shift + scale * final_v,
                     normal_set.at(vnj), normal_set.at(vni), final_vn,
                     current_material_index);

               obj_set.push_back(tr);

               number_of_polygons ++;
               number_of_triangles ++;

               if (bounding_enabled) {
                  content.push_back(tr);
               }
               
            }
            
            // Adding the last triangle
            const texture_info info =
               apply_texture ?

               texture_info(current_texture_index,
                  {uv_coord_set.at(last_vt).x, 1-uv_coord_set.at(last_vt).y,
                  uv_coord_set.at(vt1).x, 1-uv_coord_set.at(vt1).y,
                  final_vt.x, 1-final_vt.y})
               :
               texture_info();

            const triangle* tr =
               apply_texture ?
               
               new triangle(
                  shift + scale * vertex_set.at(last_v),
                  shift + scale * vertex_set.at(v1),
                  shift + scale * final_v,
                  normal_set.at(last_vn), normal_set.at(vn1), final_vn,
                  current_material_index
                  , info)
               :
               new triangle(
                  shift + scale * vertex_set.at(last_v),
                  shift + scale * vertex_set.at(v1),
                  shift + scale * final_v,
                  normal_set.at(last_vn), normal_set.at(vn1), final_vn,
                  current_material_index
                  );

            obj_set.push_back(tr);

            number_of_polygons ++;
            number_of_triangles ++;

            if (bounding_enabled) {
               content.push_back(tr);
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
         output_bd = children.at(0);
      }
      else {
         output_bd = containing_bounding_any(children);
      }
   }

   printf("\r%s successfully loaded:\n", file_name);
   printf("%u vertices, %u polygons (%u triangles, %u quads)\n",
      number_of_vertices, number_of_polygons, number_of_triangles, number_of_quads);
   fflush(stdout);

   return true;
}

