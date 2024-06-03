#pragma once

#include <vector>
#include "objects/object.hpp"
#include "objects/bounding.hpp"
#include "material/texture.hpp"
#include "screen/color.hpp"
#include "auxiliary/randomgen.hpp"
#include "camera.hpp"


class scene {
    public:
        /* Set of all the objects in the scene */
        /* Storing pointers allow the overridden methods send and intersect (from sphere, plane, triangle...)
           to be executed instead of the base (object) one */
        std::vector<const object*> object_set;

        /* Set of the first-level bounding boxes */
        std::vector<const bounding*> bounding_set;

        /* Set containing all the textures from the scene */
        std::vector<texture> texture_set;

        /* Set containing all the materials from the scene */
        std::vector<material> material_set;


        // Color of the background
        rt::color background;

        // Screen parameters
        int width;
        int height;

        // Camera parameters
        camera cam;

        // Random number generator
        randomgen rg;

        // Triangles are grouped by the given number in the bounding box tree-search method
        unsigned int polygons_per_bounding;

        /* Constructor */

        /* Main constructor */
        scene(const std::vector<const object*>& object_set,
            const std::vector<const bounding*>& bounding_set,
            const std::vector<texture>& texture_set,
            const std::vector<material>& material_set,
            const rt::color& background,
            const int width, const int height,
            const camera& cam,
            const unsigned int polygons_per_bounding);

        /* For testing purposes: copy constructor */
        scene(const scene& sc);

        ~scene();

        /*************************************************************************************/

        /* Ray-scene intersection */
        /* Linear search through the objects of the scene */
        std::optional<hit> find_closest_object(ray& r) const;

        /* Tree-search through the bounding boxes */
        std::optional<hit> find_closest_object_bounding(ray& r) const;
};