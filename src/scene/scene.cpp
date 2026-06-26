#include "scene/scene.hpp"

#include "file_readers/parsers/scene_parser.hpp"
#include "auxiliary/custom_stack.hpp"

#include <optional>

scene::scene(
    std::vector<const object*>&& object_set,
    std::vector<const bounding*>&& bounding_set,
    std::vector<texture>&& texture_set,
    std::vector<normal_map>&& normal_map_set,
    std::vector<material>&& material_set,
    std::vector<texture_info>&& texture_info_set,
    background_container&& background,
    camera&& cam,
    const int width, const int height,
    const unsigned int polygons_per_bounding,
    const real gamma)
    :
    object_set      (std::move(object_set)),
    bounding_set    (std::move(bounding_set)),
    texture_set     (std::move(texture_set)),
    normal_map_set  (std::move(normal_map_set)),
    material_set    (std::move(material_set)),
    texture_info_set(std::move(texture_info_set)),
    background      (std::move(background)),
    cam             (std::move(cam)),
    width(width), height(height),
    polygons_per_bounding(polygons_per_bounding),
    gamma(gamma) {}


scene::~scene() {

    /* Destruction of the objects located on the heap */
    for (const object* obj : object_set) {
        delete obj;
    }

    /* Destruction of the boundings with a breadth-first search */
    custom_stack<const bounding*> bd_stack(50);
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
std::optional<hit> scene::find_closest_object(const ray& r) const {
    
    real distance_to_closest = infinity;
    unsigned int closest_obj_index = EMPTY_INDEX;

    // Looking for the closest object
    for (unsigned int i = 0; const object* obj : object_set) {
        
        const real d = obj->measure_distance(r);
        
        /* d is the distance between the origin of the ray and the
           intersection point with the object */

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
    
    //std::stack<const bounding*> bounding_stack;
    static thread_local custom_stack<const bounding*> bounding_stack(50);
    bounding_stack.set_empty();

    /* Pass through the set of first-level bounding boxes */
    for (const bounding* const bd : bounding_set) {
        bd->check_box(r, distance_to_closest, closest_obj, bounding_stack);
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
        
        bd->check_box_next(r, distance_to_closest, closest_obj, bounding_stack,
            bd_stored, next_bounding);
    }

    /* Finally, return the hit corresponding to the closest object intersected by the ray */
    return (closest_obj != nullptr) ?
          std::optional<hit>(closest_obj->compute_intersection(r, distance_to_closest))
        : std::nullopt;
}

/* Returns the color of the pixel associated with UV-coordinates u, v */
const rt::color& scene::sample_texture(const unsigned int texture_info_index, const barycentric_info& bary) const {
    
    const texture_info& ti = texture_info_set[texture_info_index];
    const auto [ u, v ] = ti.get_barycenter(bary);
    return texture_set[ti.texture_index].get_color(u, v);

    /* HERE: we can introduce texture filtering */
}

map_sample scene::sample_maps(const unsigned int texture_info_index, const barycentric_info& bary,
    const rt::color& default_color, const rt::vector& default_vector, const real /*default_smoothness*/) const {

    const texture_info& ti = texture_info_set[texture_info_index];
    const auto [ u, v ] = ti.get_barycenter(bary);
    const rt::color& t_col = (ti.has_texture_information()) ?
          texture_set[ti.texture_index].get_color(u, v)
        : default_color;
    const rt::vector& n_vec = (ti.has_normal_information()) ?
          normal_map_set[ti.normal_map_index].get_tangent_space_normal(u, v)
        : default_vector;
    // const real smoothness = (ti.roughness_map_index.has_value()) ?
    //       1.0f - roughness_map_set[ti.roughness_map_index.value()].get_roughness(uvc.u, uvc.v)
    //     : default_smoothness;
    // const real displacement = displacement_map_set[ti.displacement_map_index].get_displacement(uvc.u, uvc.v);

    return map_sample(t_col, n_vec//,
        // smoothness,
        // displacement
    );
}