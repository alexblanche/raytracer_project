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