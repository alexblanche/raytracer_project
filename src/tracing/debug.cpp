#include "tracing/debug.hpp"

#include "screen/screen.hpp"

#include <iostream>
#include <array>

void print_hit_info(const scene& scene, int x, int y) {

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

    if (obj->is_textured()) {
        const texture_info& ti = scene.get_texture_info(obj);
        printf("Texture info: ");
        const auto& [ u0, v0, u1, v1, u2, v2, u3, v3 ] = ti.uv_coordinates;
        if (dynamic_cast<const triangle*>(obj))
            printf("(%lf, %lf) (%lf, %lf) (%lf, %lf)\n",
                u0, v0, u1, v1, u2, v2);
        else
            printf("(%lf, %lf) (%lf, %lf) (%lf, %lf) (%lf, %lf)\n",
                u0, v0, u1, v1, u2, v2, u3, v3);
    }
}


////////////////////////////////////////////////////////////////////////////

struct bd_depth {
    const bounding* bd;
    unsigned int depth;
};

static void check_box(const bounding* const bd, const unsigned int current_depth, const ray& r,
    custom_stack<bd_depth>& bounding_stack,
    real& distance_to_closest, unsigned int& search_depth) {
    
    if constexpr (std::is_same_v<bounding::box_type, box>) {
        if (bd->b != nullptr
            && reinterpret_cast<const box*>(bd->b.get())->is_hit_with_distance(r) >= distance_to_closest)
            return;
    }
    else if constexpr (std::is_same_v<bounding::box_type, aabb>) {
        if (bd->b != nullptr
            && reinterpret_cast<const aabb*>(bd->b.get())->measure_distance(r) >= distance_to_closest)
            return;
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
}

static unsigned int compute_search_depth(const scene& scene, const ray& r) {

    real distance_to_closest = infinity;
    unsigned int search_depth = 0;
    
    static custom_stack<bd_depth> bounding_stack;
    bounding_stack.set_empty();

    /* Pass through the set of first-level bounding boxes */
    for (const bounding* const bd : scene.bounding_set) {
        bounding_stack.emplace(bd, 1);
    }

    while (not bounding_stack.empty()) {

        const auto [ bd, depth ] = bounding_stack.pop();
        check_box(bd, depth, r, bounding_stack, distance_to_closest, search_depth);
    }

    return search_depth;
}

constexpr std::array depth_color_array = {
    rt::BLACK,
    rt::color(50, 50, 50),
    rt::color(100,100,100),
    rt::color(180,180,180),
    rt::BLUE,
    rt::color(128, 0, 255),
    rt::color(255, 0, 255),
    rt::color(255, 0, 128),
    rt::RED,
    rt::color(255, 128, 0),
    rt::color(255, 255, 0),
    rt::color(128, 255, 0),
    rt::GREEN,
    rt::color(128, 255, 128),
    rt::WHITE
};

static inline const rt::color& depth_color(const unsigned int depth) {
    
    constexpr unsigned int max_index = depth_color_array.size() - 1;
    return depth_color_array[std::min(depth, max_index)];
}

image display_search_depth(const scene& scene) {

    image img(scene.width, scene.height);

    for (int j = 0; j < scene.height; j++) {
        for (int i = 0; i < scene.width; i++) {
            const ray r = scene.cam.gen_ray_classic(i, j, 1);
            const unsigned int depth = compute_search_depth(scene, r);

            img[scene.height - j - 1, i] = depth_color(depth);
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

    if (bd->b == nullptr)
        return;
    
    const auto [ min_x, max_x, min_y, max_y, min_z, max_z ] = bd->get_min_max_coord();

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

void draw_bounding_boxes(const scene& scene, const unsigned int max_depth) {

    image img = display_search_depth(scene);
    rt::screen scr(img);

    custom_stack<bd_depth> bounding_stack;
    bounding_stack.set_empty();

    for (const bounding* const bd : scene.bounding_set) {
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