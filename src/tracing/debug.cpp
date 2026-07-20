#include "tracing/debug.hpp"

#include <iostream>

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

    printf("Object: ");
    obj->print();
}