#include "tracing/debug.hpp"

#include "screen/screen.hpp"

#include <iostream>
#include <array>

void debug::print_hit_info(const scene& scene, int x, int y) {

    const ray r = scene.cam.gen_ray_classic(x, y, 1);
    const std::optional<hit> opt_h = scene.find_closest_object(r);
    if (not opt_h.has_value()) {
        printf("Background\n");
        return;
    }

    const hit& h = opt_h.value();
    const rt::vector& hit_point = h.get_point();
    const object* const obj = h.get_object();

    printf("Hit point: ");
    hit_point.print();
    printf("\n");

    obj->print();

    if (not obj->is_textured())
        return;

    const mapping_info* const mi = scene.orientation_containers.get_mapping_info(
        obj->get_orientation_info_index(), h.get_object_type()
    );

    printf("Orientation: ");

    using enum object_type;
    switch (h.get_object_type()) {
        case Triangle: {
            const triangle::orientation& o = *static_cast<const triangle::orientation*>(mi);
            printf("index = %u, tangent = ", o.index);
            o.tangent.print();
            printf(", bitangent = ");
            o.bitangent.print();
            const auto& [ uv0, uv1, uv2 ] = o.uv;
            printf("\nuv = (%lf, %lf) (%lf, %lf) (%lf, %lf)\n",
                uv0.u, uv0.v, uv1.u, uv1.v, uv2.u, uv2.v);
            break;
        }
        case Quad: {
            const quad::orientation& o = *static_cast<const quad::orientation*>(mi);
            printf("index = %u, tangent = ", o.index);
            o.tangent.print();
            printf(", bitangent = ");
            o.bitangent.print();
            const auto& [ uv0, uv1, uv2, uv3 ] = o.uv;
            printf("\nuv = (%lf, %lf) (%lf, %lf) (%lf, %lf) (%lf, %lf)\n",
                uv0.u, uv0.v, uv1.u, uv1.v, uv2.u, uv2.v, uv3.u, uv3.v);
            break;
        }
        case Sphere: {
            const sphere::orientation& o = *static_cast<const sphere::orientation*>(mi);
            printf("index = %u\n", o.index);
            if constexpr (std::is_same_v<decltype(o.matrix), linalg::mat3<linalg::mat_type::Col>>) {
                const auto& [ c1, c2, c3 ] = o.matrix;
                printf("matrix = %lf %lf %lf",   c1.x, c2.x, c3.x);
                printf("         %lf %lf %lf",   c1.y, c2.y, c3.y);
                printf("         %lf %lf %lf\n", c1.z, c2.z, c3.z);
            }
            else {
                const auto& [ r1, r2, r3 ] = o.matrix;
                printf("matrix = %lf %lf %lf",   r1.x, r1.y, r1.z);
                printf("         %lf %lf %lf",   r2.x, r2.y, r2.z);
                printf("         %lf %lf %lf\n", r3.x, r3.y, r3.z);
            }
            break;
        }
        case Plane: {
            const plane::orientation& o = *static_cast<const plane::orientation*>(mi);
            printf("index = %u, inv_scale = %lf, right_dir = ", o.index, o.inv_texture_scale);
            o.right_dir.print();
            printf(", down_dir = ");
            o.down_dir.print();
            printf("\n");
            break;
        }
        default:
            break;
    }
    static_assert(TODO_BOX_TEXTURING);
    static_assert(TODO_CYLINDER_TEXTURING);
}


////////////////////////////////////////////////////////////////////////////

struct bd_depth {
    const bounding* bd;
    unsigned int depth;
};

unsigned int debug::compute_search_depth(const scene& scene, const ray& r) {

    real distance_to_closest = infinity;
    unsigned int search_depth = 0;
    
    static custom_stack<bd_depth> bounding_stack;
    bounding_stack.set_empty();

    /* Pass through the set of first-level bounding boxes */
    for (const bounding* const bd : scene.bvh_.bounding_set) {
        bounding_stack.emplace(bd, 1);
    }

    const auto check_box = [&] (const bounding* const bd, const unsigned int current_depth) {
        if (bd->box_index != EMPTY_INDEX) {

            const bounding::box_type& b = scene.bvh_.box_set[bd->box_index];

            if constexpr (std::is_same_v<bounding::box_type, box>) {
                if (reinterpret_cast<const box*>(&b)->is_hit_with_distance(r) >= distance_to_closest)
                    return;
            }
            else if constexpr (std::is_same_v<bounding::box_type, aabb>) {
                if (reinterpret_cast<const aabb*>(&b)->measure_distance(r) >= distance_to_closest)
                    return;
            }
        }

        using enum bounding::node_type;
        switch (bd->type) {
            
            case InternalNode: {
                const auto& children = bd->get_children();
                for (const bounding* bd : children) {
                    bounding_stack.emplace(bd, current_depth + 1);
                }
                break;
            }

            case TerminalNode: {
                const auto& content = bd->get_content();
                for (const object* const obj : content) {
                    const real d = obj->measure_distance(r);
                    if (d < distance_to_closest) {
                        distance_to_closest = d;
                        search_depth = current_depth;
                    }
                }
                break;
            }
        }
    };

    while (not bounding_stack.empty()) {

        const auto [ bd, depth ] = bounding_stack.pop();
        check_box(bd, depth);
    }

    return search_depth;
}

constexpr std::array depth_color_array = {
    rt::BLACK,
    rt::color(50, 50, 50),
    rt::color(100,100,100), // Gray
    rt::color(180,180,180),
    rt::BLUE,
    rt::color(128, 0, 255),
    rt::color(255, 0, 255), // Purple
    rt::color(255, 0, 128),
    rt::RED,
    rt::color(255, 128, 0),
    rt::color(255, 255, 0), // Yellow
    rt::color(128, 255, 0),
    rt::GREEN,
    rt::color(128, 255, 128),
    rt::WHITE
};

static inline const rt::color& depth_color(const unsigned int depth) {
    
    constexpr unsigned int max_index = depth_color_array.size() - 1;
    return depth_color_array[std::min(depth, max_index)];
}

image debug::display_search_depth(const scene& scene) {

    image img(scene.width, scene.height);

    for (int j = 0; j < scene.height; j++) {
        for (int i = 0; i < scene.width; i++) {
            const ray r = scene.cam.gen_ray_classic(i, j, 1);
            const unsigned int depth = compute_search_depth(scene, r);

            img[j, i] = depth_color(depth);
        }
    }

    img.increase_sample_count();

    rt::screen scr(img);
    scr.refresh();
    runtime_debugger debug;
    scr.wait_keyboard_event(debug);
    return img;
}

static void draw_bounding_box(const rt::screen& scr, const scene& scene,
    const bounding* bd, const unsigned int depth) {

    if (bd->box_index == EMPTY_INDEX)
        return;
    
    const auto [ min_x, max_x, min_y, max_y, min_z, max_z ] = scene.bvh_.get_min_max_coord(bd);

    /*
    printf("Box: (b = %p) depth: %u, x: [%lf; %lf]; y: [%lf; %lf]; z: [%lf; %lf]",
            static_cast<void*>(bd->b.get()), depth, min_x, max_x, min_y, max_y, min_z, max_z);
    printf(" (content: ");
    if (bd->type == bounding::node_type::InternalNode)
        printf("%zu children)\n", bd->get_children().size());
    else
        printf("%zu polygons)\n", bd->get_content().size());
    */

    const std::array vertices = {
        rt::vector(min_x, min_y, min_z),
        rt::vector(min_x, min_y, max_z),
        rt::vector(min_x, max_y, max_z),
        rt::vector(min_x, max_y, min_z),
        rt::vector(max_x, min_y, min_z),
        rt::vector(max_x, min_y, max_z),
        rt::vector(max_x, max_y, max_z),
        rt::vector(max_x, max_y, min_z)
    };

    static_assert(vertices.size() == 8);
    std::array<rt::point, 8> proj;
    for (int i = 0; const rt::vector& v : vertices)
        proj[i++] = scene.cam.project(v, scene.width, scene.height);

    // for (const auto& p : proj)
    //     std::cout << p.x << ", " << p.y << std::endl;

    // for (rt::point& p : proj)
    //     p = { std::clamp(p.x, 0, scene.width - 1), std::clamp(p.y, 0, scene.height - 1) };

    const auto& [ p0, p1, p2, p3, q0, q1, q2, q3 ] = proj;

    const std::array points = {
        p0, p1, p2, p3, p0, q0,
        q1, p1, q1, q2, p2, q2, q3, p3, q3, q0
    };

    scr.draw_lines(points, depth_color(depth));
}

void debug::draw_bounding_boxes(const scene& scene, const unsigned int max_depth) {

    image img = display_search_depth(scene);
    rt::screen scr(img);

    custom_stack<bd_depth> bounding_stack;
    bounding_stack.set_empty();

    for (const bounding* const bd : scene.bvh_.bounding_set) {
        bounding_stack.emplace(bd, 1);
    }

    scr.fast_copy(1);
    scr.clear();
    scr.render_texture();

    while (not bounding_stack.empty()) {

        const auto [ bd, depth ] = bounding_stack.pop();
        if (depth > max_depth)
            continue;

        if (bd->type == bounding::node_type::InternalNode) {
            const auto& children = bd->get_children();
            for (const bounding* bd : children)
                bounding_stack.emplace(bd, depth + 1);
        }

        draw_bounding_box(scr, scene, bd, depth);
    }

    scr.update();
    runtime_debugger debug;
    scr.wait_keyboard_event(debug);
}