#pragma once

#include <vector>
#include "objects/object.hpp"
#include "objects/bounding.hpp"
#include "material/texture.hpp"
#include "auxiliary/randomgen.hpp"
#include "camera.hpp"
#include "material/background.hpp"
#include "material/normal_map.hpp"

/* Struct containing all info from a map sample */
struct map_sample {
    const rt::color& texture_color;
    const rt::vector& normal_map_vector;
    // real roughness;
    // real displacement;

    map_sample(const rt::color& texture_color,
        const rt::vector& normal_map_vector//,
        // real roughness;
        // real displacement;
    )
    : texture_color(texture_color),
    normal_map_vector(normal_map_vector)//,
    //roughness(roughness), displacement(displacement)
    {}
};

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

        /* Set containing all the normal maps from the scene */
        std::vector<normal_map> normal_map_set;

        /* Set containing all the materials from the scene */
        std::vector<material> material_set;

        // Color or texture of the background
        background_container background;

        // Screen parameters
        int width;
        int height;

        // Camera parameters
        camera cam;

        // Triangles are grouped by the given number in the bounding box tree-search method
        unsigned int polygons_per_bounding;

        real gamma;

        /* Constructor */

        /* Constructor with only background color */
        scene(std::vector<const object*>&& object_set,
            const std::vector<const bounding*>& bounding_set,
            std::vector<texture>&& texture_set,
            std::vector<normal_map>&& normal_map_set,
            std::vector<material>&& material_set,
            const rt::color& bg_color,
            const int width, const int height,
            const camera& cam,
            const unsigned int polygons_per_bounding,
            const real gamma);

        /* Constructor with background texture and optional background color */
        scene(std::vector<const object*>&& object_set,
            const std::vector<const bounding*>& bounding_set,
            std::vector<texture>&& texture_set,
            std::vector<normal_map>&& normal_map_set,
            std::vector<material>&& material_set,
            texture&& bg_texture, const real bg_rx, const real bg_ry, const real bg_rz,
            const int width, const int height,
            const camera& cam,
            const unsigned int polygons_per_bounding,
            const real gamma);

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

        inline std::optional<hit> find_closest(ray& r, const bool bounding_method) {
            return bounding_method ?
                find_closest_object_bounding(r)
                :
                find_closest_object(r);
        }

        /* Returns the color of the pixel associated with UV-coordinates u, v */
        const rt::color& sample_texture(const texture_info& ti, const barycentric_info& bary) const;

        /* Sampling maps */
        map_sample sample_maps(const texture_info& ti, const barycentric_info& bary,
            const rt::color& default_color, const rt::vector& default_normal, const real default_reflectivity = 0.0f) const;
};