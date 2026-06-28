#pragma once

#include "scene/objects/object.hpp"
#include "scene/objects/bounding.hpp"
#include "scene/material/texture.hpp"
#include "auxiliary/randomgen.hpp"
#include "scene/camera.hpp"
#include "scene/material/background.hpp"
#include "scene/material/normal_map.hpp"
#include "scene/material/material.hpp"

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

enum class bvh_option {
    Enabled, Disabled
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

        /* Set containing all the texture_info structures for the objects */
        std::vector<texture_info> texture_info_set;

        // Color or texture of the background
        background_container background;

        // Camera parameters
        camera cam;

        // Screen parameters
        int width;
        int height;

        // Triangles are grouped by the given number in the bounding box tree-search method
        unsigned int polygons_per_bounding;

        real gamma;
        

        /* Constructor with background texture and optional background color */
        scene(
            std::vector<const object*>&& object_set,
            std::vector<const bounding*>&& bounding_set,
            std::vector<texture>&& texture_set,
            std::vector<normal_map>&& normal_map_set,
            std::vector<material>&& material_set,
            std::vector<texture_info>&& texture_info_set,
            background_container&& background,
            camera&& cam,
            int width, int height,
            unsigned int polygons_per_bounding,
            real gamma
        );

        scene(scene&&) = default;

        scene(const scene&)             = delete;
        scene& operator=(const scene&)  = delete;
        scene& operator=(scene&&)       = delete;

        ~scene();

        /*************************************************************************************/

        /* Ray-scene intersection */
        /* Linear search through the objects of the scene */
        std::optional<hit> find_closest_object(const ray& r) const;

        /* Tree-search through the bounding boxes */
        std::optional<hit> find_closest_object_bounding(const ray& r) const;

        inline std::optional<hit> find_closest(const ray& r, const bvh_option bvh) const {
            using enum bvh_option;
            switch (bvh) {
                case Enabled:
                    return find_closest_object_bounding(r);
                case Disabled:
                    return find_closest_object(r);
            }
        }

        /* Returns the color of the pixel associated with UV-coordinates u, v */
        const rt::color& sample_texture(unsigned int texture_info_index, const barycentric_info& bary) const;

        /* Sampling maps */
        map_sample sample_maps(unsigned int texture_info_index, const barycentric_info& bary,
            const rt::color& default_color, const rt::vector& default_normal, real default_reflectivity = 0.0_r) const;

        inline const texture_info& get_texture_info (const object* obj) const {
            return texture_info_set[obj->get_texture_info_index()];
        }
};