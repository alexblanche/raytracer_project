#include "file_readers/obj_parser.hpp"
#include "light/vector.hpp"

#include "scene/objects/triangle.hpp"
#include "scene/objects/quad.hpp"

#include <stdio.h>
#include <string.h>
#include <string>

/* Wavefront .obj file parser */
/* Only handles .obj files made up of triangles and quads, for now.
   In the future, maybe split polygons with >= 5 sides into triangles */

/* Parses .obj file file_name. Triangles and quads are added to obj_set,
   with material indices (defined with the keyword usemtl) found in material_names
   
   - Only one texture is handled.

   - Object names (o), polygon groups (g), smooth shading (s), lines (l) are ignored

   - The object is scaled with the factor scale, and shifted by the vector shift

   Returns true if the operation was successful
 */
bool parse_obj_file(const char* file_name, std::vector<const object*>& obj_set,
   const unsigned int texture_index, std::vector<string>& material_names,
   const double& scale, const rt::vector& shift) {

   printf("Parsing obj file...\n");

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

   /* Parsing loop */
   while (not feof(file)) {

      // longest items are usemtl, mtllib
      char s[7];
      if (fscanf(file, "%6s ", s) != 1) {
         break;
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

            obj_set.push_back(
               new triangle(
                  shift + scale * vertex_set.at(v1),
                  shift + scale * vertex_set.at(v2),
                  shift + scale * vertex_set.at(v3),
                  normal_set.at(vn1), normal_set.at(vn2), normal_set.at(vn3),
                  current_material_index, info)
            );

            number_of_polygons ++;
            number_of_triangles ++;
         }
         else if (ret == 12) {
            /* Quad */
            const texture_info info(texture_index,
               {uv_coord_set.at(vt1).x, 1-uv_coord_set.at(vt1).y,
               uv_coord_set.at(vt2).x, 1-uv_coord_set.at(vt2).y,
               uv_coord_set.at(vt3).x, 1-uv_coord_set.at(vt3).y,
               uv_coord_set.at(vt4).x, 1-uv_coord_set.at(vt4).y}
            );

            
            obj_set.push_back(
               new quad(
                  shift + scale * vertex_set.at(v1),
                  shift + scale * vertex_set.at(v2),
                  shift + scale * vertex_set.at(v3),
                  shift + scale * vertex_set.at(v4),
                  normal_set.at(vn1), normal_set.at(vn2), normal_set.at(vn3), normal_set.at(vn4),
                  current_material_index, info)
            );
            

            number_of_polygons ++;
            number_of_quads ++;
         }
         else {
            fclose(file);
            printf("Parsing error in file %s (face definition)\n", file_name);
            return false;
         }
      }
   }
   
   fclose(file);

   printf("%s successfully loaded:\n", file_name);
   printf("%u vertices, %u polygons (%u triangles, %u quads)\n",
      number_of_vertices, number_of_polygons, number_of_triangles, number_of_quads);

   return true;
}

