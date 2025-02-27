#pragma once

#include <vector>
#include "objects/object.hpp"
#include "objects/bounding.hpp"
#include "material/texture.hpp"
#include "screen/color.hpp"
#include "auxiliary/randomgen.hpp"
#include "camera.hpp"
#include "material/background.hpp"

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

        // Color or texture of the background
        background_container background;

        // Screen parameters
        int width;
        int height;

        // Camera parameters
        camera cam;

        // Random number generator
        randomgen rg;

        // Triangles are grouped by the given number in the bounding box tree-search method
        unsigned int polygons_per_bounding;

        real gamma;

        /* Constructor */

        /* Constructor with only background color */
        scene(std::vector<const object*>&& object_set,
            const std::vector<const bounding*>& bounding_set,
            std::vector<texture>&& texture_set,
            std::vector<material>&& material_set,
            const rt::color& bg_color,
            const int width, const int height,
            const camera& cam,
            const unsigned int polygons_per_bounding,
            const real std_dev_anti_aliasing, const real gamma);

        /* Constructor with background texture and optional background color */
        scene(std::vector<const object*>&& object_set,
            const std::vector<const bounding*>& bounding_set,
            std::vector<texture>&& texture_set,
            std::vector<material>&& material_set,
            texture&& bg_texture, const real bg_rx, const real bg_ry, const real bg_rz,
            const int width, const int height,
            const camera& cam,
            const unsigned int polygons_per_bounding,
            const real std_dev_anti_aliasing, const real gamma);

        /* For testing purposes: forbidding copy */
        scene(const scene&) = delete;

        scene& operator=(const scene&) = delete;

        /* Only move operations allowed */
        scene(scene&&) = default;

        scene& operator=(scene&&) = default;

        ~scene();

        /*************************************************************************************/

        /* Ray-scene intersection */
        /* Linear search through the objects of the scene */
        std::optional<hit> find_closest_object(ray& r) const;

        /* Tree-search through the bounding boxes */
        std::optional<hit> find_closest_object_bounding(ray& r) const;
};