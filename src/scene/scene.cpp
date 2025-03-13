#include "scene/scene.hpp"

#include "file_readers/scene_parser.hpp"

#include <optional>
#include <limits>
std::numeric_limits<real> realscene;
const real infinity = realscene.infinity();


/* Constructor with only background color */
scene::scene(std::vector<const object*>&& object_set,
    const std::vector<const bounding*>& bounding_set,
    std::vector<texture>&& texture_set,
    std::vector<normal_map>&& normal_map_set,
    std::vector<material>&& material_set,
    const rt::color& bg_color,
    const int width, const int height,
    const camera& cam,
    const unsigned int polygons_per_bounding,
    const real std_dev_anti_aliasing, const real gamma)

    : object_set(std::move(object_set)), bounding_set(bounding_set),
    texture_set(std::move(texture_set)), normal_map_set(std::move(normal_map_set)),
    material_set(std::move(material_set)),
    background(background_container(bg_color)),
    width(width), height(height),
    cam(cam), rg(randomgen(std_dev_anti_aliasing)), polygons_per_bounding(polygons_per_bounding),
    gamma(gamma) {}


/* Constructor with background texture */
scene::scene(std::vector<const object*>&& object_set,
    const std::vector<const bounding*>& bounding_set,
    std::vector<texture>&& texture_set,
    std::vector<normal_map>&& normal_map_set,
    std::vector<material>&& material_set,
    texture&& bg_texture, const real rx, const real ry, const real rz,
    const int width, const int height,
    const camera& cam,
    const unsigned int polygons_per_bounding,
    const real std_dev_anti_aliasing, const real gamma)

    : object_set(std::move(object_set)), bounding_set(bounding_set),
    texture_set(std::move(texture_set)), normal_map_set(std::move(normal_map_set)),
    material_set(std::move(material_set)),
    background(background_container(std::move(bg_texture), rx, ry, rz)),
    width(width), height(height),
    cam(cam), rg(randomgen(std_dev_anti_aliasing)), polygons_per_bounding(polygons_per_bounding),
    gamma(gamma) {}



/*********************************************************************/

/* Destructor */

scene::~scene() {
    // printf("Destroying scene\n");

    /* Destruction of the objects located on the heap */
    for (const object* obj : object_set) {
        delete(obj);
    }

    /* Recursive destruction of the bounding boxes */
    std::stack<const bounding*> bd_stack;
    for (const bounding* bd : bounding_set) {
        bd_stack.push(bd);
    }
    while (not bd_stack.empty()) {
        const bounding* bd = bd_stack.top();
        bd_stack.pop();

        std::vector<const bounding*> bd_children = bd->get_children();
        for (const bounding* bd : bd_children) {
            bd_stack.push(bd);
        }
        delete(bd);
    }
}

/*********************************************************************/

/*** Ray-scene intersection ***/

/* Linear search through the objects of the scene */
std::optional<hit> scene::find_closest_object(ray& r) const {
    
    real distance_to_closest = infinity;
    std::optional<size_t> closest_obj_index = std::nullopt;

    // Looking for the closest object
    for (size_t i = 0; i < object_set.size(); i++) {
        
        // We do not test the intersection with the object the rays is cast from
        const std::optional<real> d = object_set[i]->measure_distance(r);
        
        /* d is the distance between the origin of the ray and the
           intersection point with the object */

        if (d.has_value() && d.value() < distance_to_closest) {
            distance_to_closest = d.value();
            closest_obj_index = i;
        }
    }

    if (closest_obj_index.has_value()) {
        return object_set[closest_obj_index.value()]->compute_intersection(r, distance_to_closest);
    }
    else {
        return std::nullopt;
    }
}

/* Tree-search through the bounding boxes */
std::optional<hit> scene::find_closest_object_bounding(ray& r) const {
    /* For all the bounding boxes in bounding::set, we do the following:
       If the bounding box is terminal, look for the object of minimum distance.
       If it is internal, if the ray intersects the box, add its children to the bounding stack.
       Then apply the same algorithm to the bounding stack, until it is empty.
       Finally, compute the hit associated with the object of minimum distance.
     */

    real distance_to_closest = infinity;
    std::optional<const object*> closest_obj = std::nullopt;
    std::stack<const bounding*> bounding_stack;

    /* Pass through the set of first-level bounding boxes */
    for (const bounding* const& bd : bounding_set) {
        bd->check_box(r, distance_to_closest, closest_obj, bounding_stack);
    }

    /* In order to avoid pushing and then immediately popping an element from bounding_stack,
       we store the last element of bd->children in next_bounding.
       The boolean bd_stored indicates whether we should pop an element, or if one is currently
       stored.
     */
    const bounding* next_bounding;
    bool bd_stored = false;

    /* Apply the same to the bounding box stack */
    while (bd_stored || (not bounding_stack.empty())) {

        const bounding* bd = bd_stored ? next_bounding : bounding_stack.top();
        if (not bd_stored) {
            bounding_stack.pop();
        }
        
        bd->check_box_next(r, distance_to_closest, closest_obj, bounding_stack,
            bd_stored, next_bounding);
    }

    /* Finally, return the hit corresponding to the closest object intersected by the ray */
    if (closest_obj.has_value()) {
        return closest_obj.value()->compute_intersection(r, distance_to_closest);
    }
    else {
        return std::nullopt;
    }
}

/* Returns the color of the pixel associated with UV-coordinates u, v */
const rt::color& scene::sample_texture(const texture_info& ti, const barycentric_info& bary) const {
    
    const uvcoord uvc = ti.get_barycenter(bary);
    return texture_set[ti.texture_index].get_color(uvc.u, uvc.v);

    /* HERE: we can introduce texture filtering */
}

map_sample scene::sample_maps(const texture_info& ti, const barycentric_info& bary) const {

    const uvcoord uvc = ti.get_barycenter(bary);
    const rt::color& t_col = texture_set[ti.texture_index].get_color(uvc.u, uvc.v);
    const rt::vector& n_vec = normal_map_set[ti.normal_map_index.value()].get_local_normal(uvc.u, uvc.v);
    // const real roughness = roughness_map_set[ti.roughness_map_index].get_roughness(uvc.u, uvc.v);
    // const real displacement = displacement_map_set[ti.displacement_map_index].get_displacement(uvc.u, uvc.v);

    return map_sample(t_col, n_vec//,
        // roughness,
        // displacement
        );
}