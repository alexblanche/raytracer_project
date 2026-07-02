#include "scene/bounding/aabb.hpp"

#include <iostream>

#include "auxiliary/utils.hpp"

real aabb::measure_distance(const ray& r) const {

    const auto& [ u, dir, inv_dir ] = r;

    const rt::vector v = u - corner;

    if ((v.x > 0.0_r && v.x < dims.x) && (v.y > 0.0_r && v.y < dims.y) && (v.z > 0.0_r && v.z < dims.z)) {
        // u is inside the box
        return 0.0_r;
    }

    /* Only ONE t can reach the box, so we return as soon as one fits */
    
    // constexpr real MAX_DIST = 1000.0_r;

    const real tx = ((std::signbit(dir.x) ? dims.x : 0.0_r) - v.x) * inv_dir.x;
    //if (tx > 0.0_r && tx < MAX_DIST) {
        const real py = std::fma(dir.y, tx, v.y);
        if (py > 0.0_r && py < dims.y) {
            const real pz = std::fma(dir.z, tx, v.z);
            if (pz > 0.0_r && pz < dims.z)
                return tx > 0.0_r ? tx : infinity;
        }
    //}
    
    const real ty = ((std::signbit(dir.y) ? dims.y : 0.0_r) - v.y) * inv_dir.y;
    //if (ty > 0.0_r && ty < MAX_DIST) {
        const real px = std::fma(dir.x, ty, v.x);
        if (px > 0.0_r && px < dims.x) {
            const real pz = std::fma(dir.z, ty, v.z);
            if (pz > 0.0_r && pz < dims.z)
                return ty > 0.0_r ? ty : infinity;
        }
    //}

    const real tz = ((std::signbit(dir.z) ? dims.z : 0.0_r) - v.z) * inv_dir.z;
    if (tz > 0.0_r) { // && tz < MAX_DIST) {
        const real px = std::fma(dir.x, tz, v.x);
        if (px > 0.0_r && px < dims.x) {
            const real py = std::fma(dir.y, tz, v.y);
            return (py > 0.0_r && py < dims.y) ? tz : infinity;
        }
    }
    
    return infinity;
}