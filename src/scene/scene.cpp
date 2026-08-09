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

/* Linear search through the objects of the scene */
/*
std::optional<hit> scene::find_closest_object__OLD(const ray& r) const {
    
    real distance_to_closest = infinity;
    unsigned int closest_obj_index = EMPTY_INDEX;

    // Looking for the closest object
    for (unsigned int i = 0; const object* obj : object_set) {
        
        const real d = obj->measure_distance(r);
        
        // d is the distance between the origin of the ray and the
           intersection point with the object

        if (d < distance_to_closest) {
            distance_to_closest = d;
            closest_obj_index = i;
        }

        i++;
    }
    
    return (closest_obj_index != EMPTY_INDEX) ?
          std::optional<hit>(object_set[closest_obj_index]->compute_intersection(r, distance_to_closest))
        : std::nullopt;
}
*/

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

/* Linear search through the objects of the scene */
std::optional<hit> scene::find_closest_object(const ray& r) const {

    using enum object_type;
    
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

/* Returns the color of the pixel associated with UV-coordinates u, v */
// const rt::color& scene::sample_texture(const unsigned int texture_info_index, const barycentric_info& bary) const {
    
//     const texture_info& ti = mapping_containers.texture_info_set[texture_info_index];
//     const auto [ u, v ] = ti.get_barycenter(bary);
//     return mapping_containers.texture_set[ti.texture_index].get_color(u, v);

//     /* HERE: we can introduce texture filtering */
// }

const rt::color& scene::sample_color(const hit& h, const material& m) const {

    const auto& [ _, comp_set, texture_set, _, _ ] = mapping_containers;

    const object* const obj = h.get_object();

    if (not obj->is_textured())
        return m.get_color();

    const mapping_info* const mi = orientation_containers.get_mapping_info(
        obj->get_orientation_info_index(), h.get_object_type()
    );
    const mapping::composition& comp = comp_set[mi->index];

    if (not comp.has_texture)
        return m.get_color();
    
    const auto [ u, v ] = obj->compute_uv(h.get_point(), mi); // virtual call, can be replaced with switch dispatch
    return texture_set[mi->index].get_color(u, v);
}

map_sample scene::sample_maps(const hit& h, const material& m) const {

    const auto& [ material_set, comp_set, texture_set, normal_map_set, _ ] = mapping_containers;

    const object* const obj = h.get_object();

    if (not obj->is_textured())
        return map_sample(m.get_color(), h.get_normal());

    const mapping_info* const mi = orientation_containers.get_mapping_info(
        obj->get_orientation_info_index(), h.get_object_type()
    );
    const mapping::index_type index = mi->index;
    const mapping::composition& comp = comp_set[index];
    
    const auto [ u, v ] = obj->compute_uv(h.get_point(), mi); // virtual dispatch, can be replaced with switch

    // std::cout << "mi = " << mi << ", index = " << index << ", " << std::endl;

    // const composition& comp = composition_set[index];
    // ... = (comp.has_texture_information) ? ... : ...;

    const rt::color& t_col = comp.has_texture ?
          texture_set[index].get_color(u, v)
        : m.get_color();
    
    // Compute the world-space normal directly? Instead of doing it in two steps?
    const rt::vector& n_vec = comp.has_normal_map ?
          normal_map_set[index].get_tangent_space_normal(u, v)
        : h.get_normal();
    
    // const real smoothness = (mi->has_roughness_map_information()) ?
    //       1.0f - roughness_map_set[index].get_roughness(u, v)
    //     : m.get_smoothness();
    static_assert(TODO_ROUGHNESS_MAP);
    
    // const real displacement = (mi->has_displacement_information()) ?
    //        displacement_map_set[index].get_displacement(u, v)
    //      : 0.0_r;
    static_assert(TODO_DISPLACEMENT_MAP);

    return map_sample(
        t_col,
        n_vec //,
        //smoothness,
        // displacement
    );
}