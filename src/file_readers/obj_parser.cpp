#include "file_readers/obj_parser.hpp"
#include "light/vector.hpp"

#include "scene/objects/bounding.hpp"
#include "scene/objects/triangle.hpp"
#include "scene/objects/quad.hpp"

#include "auxiliary/clustering.hpp"

#include <stdio.h>
#include <string.h>
#include <string>



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

bool parse_obj_file(const char* file_name, std::vector<const object*>& obj_set,
   const unsigned int texture_index, std::vector<string>& material_names,
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

   /* Storage */
   /* All indices start at 1, so the first vector is unused */
   
   /* Vertices of the object */
   std::vector<rt::vector> vertex_set = {rt::vector()};

   /* UV-coordinates ("vt"), only the first two attributes (x, y) of the vectors are used */
   std::vector<rt::vector> uv_coord_set = {rt::vector()};

   /* Vertex normals */
   std::vector<rt::vector> normal_set = {rt::vector()};

   unsigned int current_material_index = -1;

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
      if (bounding_enabled && strcmp(s, "o") == 0 && content.size() != 0) {

         // First method: put the whole group in one bounding
         // const bounding* bd = containing_objects(content);

         // Second method: create a bounding hierarchy containing all the nodes
         /* Heuristic: each group is a depth 1 node in the global bounding box hierarchy */
         const bounding* bd = create_bounding_hierarchy(content, polygons_per_bounding);
         display_hierarchy_properties(bd);
         children.push_back(bd);
         content.clear();
      }

      /* Commented line, or ignored command */
      if (strcmp(s, "#") == 0
         || strcmp(s, "s") == 0
         || strcmp(s, "g") == 0
         || strcmp(s, "l") == 0
         || strcmp(s, "o") == 0
         || strcmp(s, "vp") == 0
         || strcmp(s, "mtllib") == 0) {

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
            uv_coord_set.push_back(rt::vector(u, v, 0));
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
         }
      }
      else if (strcmp(s, "f") == 0) {
         /* Face declaration */

         /* 3 or 4 vertices will be parsed */
         unsigned int v1, vt1, vn1, v2, vt2, vn2, v3, vt3, vn3, v4, vt4, vn4;
         const int ret = fscanf(file, "%u/%u/%u %u/%u/%u %u/%u/%u %u/%u/%u",
            &v1, &vt1, &vn1, &v2, &vt2, &vn2, &v3, &vt3, &vn3, &v4, &vt4, &vn4);

         if (ret == 9) {
            /* Triangle */
            const texture_info info(texture_index,
               {uv_coord_set.at(vt1).x, 1-uv_coord_set.at(vt1).y,
               uv_coord_set.at(vt2).x, 1-uv_coord_set.at(vt2).y,
               uv_coord_set.at(vt3).x, 1-uv_coord_set.at(vt3).y}
            );

            const triangle* tr = new triangle(
               shift + scale * vertex_set.at(v1),
               shift + scale * vertex_set.at(v2),
               shift + scale * vertex_set.at(v3),
               normal_set.at(vn1), normal_set.at(vn2), normal_set.at(vn3),
               current_material_index, info
            );

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

               const texture_info info12(texture_index,
                  {uv_coord_set.at(vt1).x, 1-uv_coord_set.at(vt1).y,
                  uv_coord_set.at(vt2).x, 1-uv_coord_set.at(vt2).y,
                  uv_coord_set.at(vt3).x, 1-uv_coord_set.at(vt3).y}
               );

               const triangle* tr1 = new triangle(
                  shift + scale * vertex_set.at(v1),
                  shift + scale * vertex_set.at(v2),
                  shift + scale * vertex_set.at(v3),
                  normal_set.at(vn1), normal_set.at(vn2), normal_set.at(vn3),
                  current_material_index, info12
               );

               obj_set.push_back(tr1);

               const texture_info info23(texture_index,
                  {uv_coord_set.at(vt1).x, 1-uv_coord_set.at(vt1).y,
                  uv_coord_set.at(vt3).x, 1-uv_coord_set.at(vt3).y,
                  uv_coord_set.at(vt4).x, 1-uv_coord_set.at(vt4).y}
               );

               const triangle* tr2 = new triangle(
                  shift + scale * vertex_set.at(v1),
                  shift + scale * vertex_set.at(v3),
                  shift + scale * vertex_set.at(v4),
                  normal_set.at(vn1), normal_set.at(vn3), normal_set.at(vn4),
                  current_material_index, info23
               );

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
               const texture_info info(texture_index,
                  {uv_coord_set.at(vt1).x, 1-uv_coord_set.at(vt1).y,
                  uv_coord_set.at(vt2).x, 1-uv_coord_set.at(vt2).y,
                  uv_coord_set.at(vt3).x, 1-uv_coord_set.at(vt3).y,
                  uv_coord_set.at(vt4).x, 1-uv_coord_set.at(vt4).y}
               );

               const quad* q = new quad(
                  shift + scale * vertex_set.at(v1),
                  shift + scale * vertex_set.at(v2),
                  shift + scale * vertex_set.at(v3),
                  shift + scale * vertex_set.at(v4),
                  normal_set.at(vn1), normal_set.at(vn2), normal_set.at(vn3), normal_set.at(vn4),
                  current_material_index, info
               );

               obj_set.push_back(q);
               
               number_of_polygons ++;
               number_of_quads ++;

               if (bounding_enabled) {
                  content.push_back(q);
               }
            }
         }
         else {
            fclose(file);
            printf("Parsing error in file %s (face definition)\n", file_name);
            return false;
         }
      }
   }
   
   fclose(file);
   
   if (bounding_enabled) {
      /* Placing the last group into a bounding */
      // const bounding* bd = containing_objects(content);
      const bounding* bd = create_bounding_hierarchy(content, polygons_per_bounding);
      display_hierarchy_properties(bd);
      children.push_back(bd);

      /* Setting the final bounding */
      if (children.size() == 1) {
         output_bd = children.at(0);
      }
      else {
         const bounding* bdch = containing_bounding_any(children);
         output_bd = bdch;
      }
   }

   printf("\r%s successfully loaded:\n", file_name);
   printf("%u vertices, %u polygons (%u triangles, %u quads)\n",
      number_of_vertices, number_of_polygons, number_of_triangles, number_of_quads);
   fflush(stdout);

   return true;
}

