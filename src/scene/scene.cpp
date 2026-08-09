#include "scene/scene.hpp"

#include "file_readers/parsers/scene_parser.hpp"
#include "auxiliary/custom_stack.hpp"

#include <optional>

static constexpr unsigned int DEFAULT_STACK_SIZE = 200;

scene::scene(
    std::vector<const object*>&&     object_set,
    std::vector<const bounding*>&&   bounding_set,
    scene::containers::object&&      object_containers,
    scene::containers::mapping&&     mapping_containers,
    scene::containers::orientation&& orientation_containers,
    camera&& cam,
    const int width, const int height,
    const unsigned int polygons_per_bounding,
    const std::optional<real> gamma) :
    
    object_set              (std::move(object_set)),
    bounding_set            (std::move(bounding_set)),
    object_containers       (std::move(object_containers)),
    mapping_containers      (std::move(mapping_containers)),
    orientation_containers  (std::move(orientation_containers)),
    cam                     (std::move(cam)),
    width(width), height(height),
    polygons_per_bounding(polygons_per_bounding),
    gamma(gamma) {}


scene::~scene() noexcept {
    /* Destruction of the boundings with a breadth-first search */
    custom_stack<const bounding*> bd_stack(DEFAULT_STACK_SIZE);
    bd_stack.push(bounding_set);

    while (not bd_stack.empty()) {
        const bounding* bd = bd_stack.pop();
        bd_stack.push(bd->get_children());
        delete bd;
    }
}

/*********************************************************************/

/*** Ray-scene intersection ***/

template<typename T>
concept Object =
       std::is_same_v<T, triangle>
    || std::is_same_v<T, quad>
    || std::is_same_v<T, sphere>
    || std::is_same_v<T, plane>
    || std::is_same_v<T, box>
    || std::is_same_v<T, cylinder>;

using enum object_type;

template<Object Obj>
object_type object_type_of() {
    if      constexpr (std::is_same_v<Obj, triangle>) return Triangle;
    else if constexpr (std::is_same_v<Obj, quad>    ) return Quad;
    else if constexpr (std::is_same_v<Obj, sphere>  ) return Sphere;
    else if constexpr (std::is_same_v<Obj, plane>   ) return Plane;
    else if constexpr (std::is_same_v<Obj, box>     ) return Box;
    else if constexpr (std::is_same_v<Obj, cylinder>) return Cylinder;
    else throw std::runtime_error("Unknown type");
}

template<Object Obj>
inline void search_closest(const std::vector<Obj>& object_type_set, const ray& r,
    real& distance_to_closest, const object*& closest_pt, object_type& closest_obj_type) {

    for (const Obj& obj : object_type_set) {

        const real d = obj.measure_distance(r);

        if (d < distance_to_closest) {
            distance_to_closest = d;
            closest_pt = &obj;
            closest_obj_type = object_type_of<Obj>();
        }
    }
}

namespace dispatch {
    std::optional<hit> compute_intersection(const object* closest_pt, const object_type closest_obj_type,
        const ray& r, const real distance_to_closest) {
        
        if (closest_pt == nullptr)
            return std::nullopt;

        switch (closest_obj_type) {
            case Triangle: return static_cast<const triangle*>(closest_pt)->compute_intersection(r, distance_to_closest);
            case Quad:     return static_cast<const quad*>    (closest_pt)->compute_intersection(r, distance_to_closest);
            case Sphere:   return static_cast<const sphere*>  (closest_pt)->compute_intersection(r, distance_to_closest);
            case Plane:    return static_cast<const plane*>   (closest_pt)->compute_intersection(r, distance_to_closest);
            case Box:      return static_cast<const box*>     (closest_pt)->compute_intersection(r, distance_to_closest);
            case Cylinder: return static_cast<const cylinder*>(closest_pt)->compute_intersection(r, distance_to_closest);
            default: throw;
        }
    }

    static inline uvcoord compute_uv(const object* obj, const object_type type,
        const rt::vector& hit_point, const mapping_info* mi) {

        switch (type) {
            case Triangle: return static_cast<const triangle*>(obj)->compute_uv(hit_point, mi);
            case Quad:     return static_cast<const quad*>    (obj)->compute_uv(hit_point, mi);
            case Sphere:   return static_cast<const sphere*>  (obj)->compute_uv(hit_point, mi);
            case Plane:    return static_cast<const plane*>   (obj)->compute_uv(hit_point, mi);
            case Box:      return static_cast<const box*>     (obj)->compute_uv(hit_point, mi);
            case Cylinder: return static_cast<const cylinder*>(obj)->compute_uv(hit_point, mi);
            default: throw;
        }
    }

    [[maybe_unused]]
    static inline rt::vector compute_normal_from_map(const object* obj, const object_type type,
        const rt::vector& tangent_space_normal, const rt::vector& local_normal, const mapping_info* mi) {
        
        switch (type) {
            case Triangle: return static_cast<const triangle*>(obj)->compute_normal_from_map(tangent_space_normal, local_normal, mi);
            case Quad:     return static_cast<const quad*>    (obj)->compute_normal_from_map(tangent_space_normal, local_normal, mi);
            case Sphere:   return static_cast<const sphere*>  (obj)->compute_normal_from_map(tangent_space_normal, local_normal, mi);
            case Plane:    return static_cast<const plane*>   (obj)->compute_normal_from_map(tangent_space_normal, local_normal, mi);
            case Box:      return static_cast<const box*>     (obj)->compute_normal_from_map(tangent_space_normal, local_normal, mi);
            case Cylinder: return static_cast<const cylinder*>(obj)->compute_normal_from_map(tangent_space_normal, local_normal, mi);
            default: throw;
        }
    }
}

/* Linear search through the objects of the scene */
std::optional<hit> scene::find_closest_object(const ray& r) const {
    
    real distance_to_closest = infinity;
    const object* closest_pt = nullptr;
    object_type closest_obj_type;

    const auto& [ triangle_set, quad_set, sphere_set, plane_set, box_set, cylinder_set ] = object_containers;

    search_closest<triangle>(triangle_set, r, distance_to_closest, closest_pt, closest_obj_type);
    search_closest<quad>    (quad_set,     r, distance_to_closest, closest_pt, closest_obj_type);
    search_closest<sphere>  (sphere_set,   r, distance_to_closest, closest_pt, closest_obj_type);
    search_closest<plane>   (plane_set,    r, distance_to_closest, closest_pt, closest_obj_type);
    search_closest<box>     (box_set,      r, distance_to_closest, closest_pt, closest_obj_type);
    search_closest<cylinder>(cylinder_set, r, distance_to_closest, closest_pt, closest_obj_type);

    return dispatch::compute_intersection(closest_pt, closest_obj_type, r, distance_to_closest);
}

/* Tree-search through the bounding boxes */
std::optional<hit> scene::find_closest_object_bounding(const ray& r) const {
    /* For all the bounding boxes in bounding::set, we do the following:
       If the bounding box is terminal, look for the object of minimum distance.
       If it is internal, if the ray intersects the box, add its children to the bounding stack.
       Then apply the same algorithm to the bounding stack, until it is empty.
       Finally, compute the hit associated with the object of minimum distance.
     */

    real distance_to_closest  = infinity;
    const object* closest_obj = nullptr;
    
    static thread_local custom_stack<const bounding*> bounding_stack(DEFAULT_STACK_SIZE);
    bounding_stack.set_empty();

    /* Pass through the set of first-level bounding boxes */
    for (const bounding* const bd : bounding_set) {
        bd->check_box(r, bounding_stack, distance_to_closest, closest_obj);
    }

    /* In order to avoid pushing and then immediately popping an element from bounding_stack,
       we store the last element of bd->children in next_bounding.
       The boolean bd_stored indicates whether we should pop an element, or if one is currently
       stored.
     */
    const bounding* next_bounding = nullptr;
    bool bd_stored = false;

    /* Apply the same to the bounding box stack */
    while (bd_stored || (not bounding_stack.empty())) {

        const bounding* bd = bd_stored ? next_bounding : bounding_stack.pop();
        
        bd->check_box_next(r, bounding_stack, distance_to_closest, closest_obj,
            bd_stored, next_bounding);
    }

    /* Finally, return the hit corresponding to the closest object intersected by the ray */
    return (closest_obj != nullptr) ?
          std::optional<hit>(closest_obj->compute_intersection(r, distance_to_closest))
        : std::nullopt;
}

const rt::color& scene::sample_color(const hit& h, const material& m) const {

    const auto& [ _, comp_set, texture_set, _, _ ] = mapping_containers;

    const object* const obj = h.get_object();

    if (not obj->is_textured())
        return m.get_color();

    const object_type type = h.get_object_type();
    const mapping_info* const mi = orientation_containers.get_mapping_info(
        obj->get_orientation_info_index(), type
    );
    const mapping::composition& comp = comp_set[mi->index];

    if (not comp.has_texture)
        return m.get_color();
    
    const auto [ u, v ] = dispatch::compute_uv(obj, type, h.get_point(), mi);
    return texture_set[mi->index].get_color(u, v);
}

map_sample scene::sample_maps(const hit& h, const material& m) const {

    const auto& [ material_set, comp_set, texture_set, normal_map_set, _ ] = mapping_containers;

    const object* const obj = h.get_object();

    if (not obj->is_textured())
        return map_sample(m.get_color(), h.get_normal());

    const object_type type = h.get_object_type();
    const mapping_info* const mi = orientation_containers.get_mapping_info(
        obj->get_orientation_info_index(), type
    );
    const mapping::index_type index = mi->index;
    const mapping::composition& comp = comp_set[index];
    
    const auto [ u, v ] = dispatch::compute_uv(obj, type, h.get_point(), mi);

    const rt::color& t_col = comp.has_texture ?
          texture_set[index].get_color(u, v)
        : m.get_color();
    
    // Tangent-space normal
    const rt::vector& n_vec = comp.has_normal_map ?
          normal_map_set[index].get_tangent_space_normal(u, v)
        : h.get_normal();
    
    // const real smoothness = (comp.has_roughness_map) ?
    //       1.0f - roughness_map_set[index].get_roughness(u, v)
    //     : m.get_smoothness();
    static_assert(TODO_ROUGHNESS_MAP);
    
    // const real displacement = (comp.has_displacement_map) ?
    //        displacement_map_set[index].get_displacement(u, v)
    //      : 0.0_r;
    static_assert(TODO_DISPLACEMENT_MAP);

    return {
        .texture_color = t_col,
        .normal_vector = comp.has_normal_map ?
              dispatch::compute_normal_from_map(obj, type, n_vec, h.get_normal(), mi)
            : n_vec
        // smoothness,
        // displacement
    };
}