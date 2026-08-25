#pragma once

#include "scene/objects/triangle.hpp"
#include "scene/objects/quad.hpp"
#include "scene/objects/sphere.hpp"
#include "scene/objects/plane.hpp"
#include "scene/objects/box.hpp"
#include "scene/objects/cylinder.hpp"

#include "scene/bvh/bvh.hpp"
#include "scene/material/texture.hpp"
#include "auxiliary/randomgen.hpp"
#include "scene/camera.hpp"
#include "scene/material/background.hpp"
#include "scene/material/normal_map.hpp"
#include "scene/material/material.hpp"

#include "scene/material/mapping.hpp"

/* Struct containing all info from a map sample */
struct map_sample {
    const rt::color& texture_color; // Reference to the color
    const rt::vector normal_vector; // The normal vector is in world-space
    // real roughness;
    // real displacement;

    static_assert(TODO_ROUGHNESS_MAP);
    static_assert(TODO_DISPLACEMENT_MAP);
};

class scene {
    public:

        struct pre_parsing_info {
            unsigned int objects    = 0;
            unsigned int triangles  = 0;
            unsigned int quads      = 0;
            unsigned int spheres    = 0;
            unsigned int planes     = 0;
            unsigned int boxes      = 0;
            unsigned int cylinders  = 0;
            unsigned int materials  = 0;
            unsigned int mappings   = 0;

            static inline const std::array<std::string, 8> keywords_array = {
                "triangle", "quad", "sphere", "plane", "box", "cylinder",
                "material", "load_mapping"
            };

            void print() const {
                std::cout << "pre_parse_info:"
                    << "\ntriangles: " << triangles
                    << "\nquads:     " << quads
                    << "\nspheres:   " << spheres
                    << "\nplanes:    " << planes
                    << "\nboxes:     " << boxes
                    << "\ncylinders: " << cylinders
                    << "\nmaterials: " << materials
                    << "\nmappings:  " << mappings
                    << std::endl;
            }

            unsigned int max_objects() const {
                return objects + quads;
            }

            unsigned int total_non_polygon_objects() const {
                return spheres + planes + boxes + cylinders;
            }
        };

        struct containers {
            struct object {
                std::vector<triangle> triangle_set;
                std::vector<quad>     quad_set;
                std::vector<sphere>   sphere_set;
                std::vector<plane>    plane_set;
                std::vector<box>      box_set;
                std::vector<cylinder> cylinder_set;

                object(const pre_parsing_info& pre_parsing_info) {
                    const auto& [ _,
                        nb_triangles,
                        nb_quads,
                        nb_spheres,
                        nb_planes,
                        nb_boxes,
                        nb_cylinders,
                        _, _
                    ] = pre_parsing_info;

                    // triangle_set must make room for split quads
                    triangle_set.reserve(nb_triangles + 2 * nb_quads);
                    quad_set    .reserve(nb_quads);
                    sphere_set  .reserve(nb_spheres);
                    plane_set   .reserve(nb_planes);
                    box_set     .reserve(nb_boxes);
                    cylinder_set.reserve(nb_cylinders);
                }
            };

            struct mapping {

                std::vector<material>   material_set;

                using comp = ::mapping::composition;
                std::vector<comp>       composition_set;
                std::vector<texture>    texture_set;
                std::vector<normal_map> normal_map_set;
                // ...
                background_container    background;

                mapping(
                    std::vector<material>&&   material_set,
                    std::vector<comp>&&       composition_set,
                    std::vector<texture>&&    texture_set,
                    std::vector<normal_map>&& normal_map_set,
                    background_container&&    background
                )
                    
                :   material_set    (std::move(material_set)),
                    composition_set (std::move(composition_set)),
                    texture_set     (std::move(texture_set)),
                    normal_map_set  (std::move(normal_map_set)),
                    // ...
                    background      (std::move(background)) {

                    static_assert(TODO_ROUGHNESS_MAP);
                    static_assert(TODO_DISPLACEMENT_MAP);
                }
            };

            struct orientation {
                std::vector<triangle::orientation> triangle_orientation_set;
                std::vector<quad    ::orientation> quad_orientation_set;
                std::vector<sphere  ::orientation> sphere_orientation_set;
                std::vector<plane   ::orientation> plane_orientation_set;
                std::vector<box     ::orientation> box_orientation_set;
                std::vector<cylinder::orientation> cylinder_orientation_set;
            
                orientation(const pre_parsing_info& pre_parsing_info) {
                    const auto& [ _,
                        nb_triangles,
                        nb_quads,
                        nb_spheres,
                        nb_planes,
                        nb_boxes,
                        nb_cylinders,
                        _, _
                    ] = pre_parsing_info;

                    // triangle_set must make room for split quads
                    triangle_orientation_set.reserve(nb_triangles + 2 * nb_quads);
                    quad_orientation_set    .reserve(nb_quads);
                    sphere_orientation_set  .reserve(nb_spheres);
                    plane_orientation_set   .reserve(nb_planes);
                    //box_orientation_set     .reserve(nb_boxes);
                    //cylinder_orientation_set.reserve(nb_cylinders);

                    static_assert(TODO_BOX_TEXTURING);
                    static_assert(TODO_CYLINDER_TEXTURING);
                }

                inline const mapping_info* get_mapping_info(const int orientation_info_index, const object_type type) const {

                    using enum object_type;
                    switch (type) {
                        case Triangle: return &triangle_orientation_set[orientation_info_index];
                        case Quad:     return &quad_orientation_set    [orientation_info_index];
                        case Sphere:   return &sphere_orientation_set  [orientation_info_index];
                        case Plane:    return &plane_orientation_set   [orientation_info_index];
                        case Box:      return &box_orientation_set     [orientation_info_index];
                        case Cylinder: return &cylinder_orientation_set[orientation_info_index];
                        default: throw;
                    }
                }
            };
        };

        /* Pointers to all the objects in the scene */
        std::vector<const object*> object_set;

        bvh bvh_;

        /* Objects, materials, textures, normal_maps */
        containers::object      object_containers;
        containers::mapping     mapping_containers;
        containers::orientation orientation_containers;

        /* Camera */
        camera cam;

        // Screen parameters
        int width;
        int height;

        std::optional<real> gamma; // Note: should be removed from scene
        

        /* Constructor with background texture and optional background color */
        scene(
            std::vector<const object*>&&     object_set,
            std::optional<bvh>&&             bvh_opt,
            scene::containers::object&&      object_containers,
            scene::containers::mapping&&     mapping_containers,
            scene::containers::orientation&& orientation_containers,
            camera&& cam,
            int width, int height,
            std::optional<real> gamma
        );

        scene(scene&&) noexcept        = default;

        scene(const scene&)            = delete;
        scene& operator=(const scene&) = delete;
        scene& operator=(scene&&)      = delete;

        /*************************************************************************************/

        /* Ray-scene intersection */
        /* Linear search through the objects of the scene */
        std::optional<hit> find_closest_object(const ray& r) const;

        inline std::optional<hit> find_intersection(const ray& r) const {
            using enum bvh::option;
            switch (bvh_.state) {
                case Enabled:
                    return bvh_.find_closest_object(r);
                case Disabled:
                    return find_closest_object(r);
                default: throw;
            }
        }

        /* Returns the color of the pixel associated with UV-coordinates u, v */
        
        /* Sampling maps */
        const rt::color& sample_color(const hit& h, const material& m) const;
        map_sample sample_maps(const hit& h, const material& m) const;
};