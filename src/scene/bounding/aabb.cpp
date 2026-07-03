#include "scene/bounding/aabb.hpp"

#include <iostream>

#include "auxiliary/utils.hpp"

real aabb::measure_distance(const ray& r) const {

    const auto& [ u, dir, inv_dir ] = r;

    const rt::vector v = u - position;

    if constexpr (type == type::Corner) {

        if ((v.x > 0.0_r && v.x < dims.x) && (v.y > 0.0_r && v.y < dims.y) && (v.z > 0.0_r && v.z < dims.z)) {
            // u is inside the box
            return 0.0_r;
        }

        /* Only one t can reach the box, so we return as soon as one fits */

        const real tx = ((std::signbit(dir.x) ? dims.x : 0.0_r) - v.x) * inv_dir.x;
        const real py = std::fma(dir.y, tx, v.y);
        if (py > 0.0_r && py < dims.y) {
            const real pz = std::fma(dir.z, tx, v.z);
            if (pz > 0.0_r && pz < dims.z)
                return tx > 0.0_r ? tx : infinity;
        }
        
        const real ty = ((std::signbit(dir.y) ? dims.y : 0.0_r) - v.y) * inv_dir.y;
        const real px = std::fma(dir.x, ty, v.x);
        if (px > 0.0_r && px < dims.x) {
            const real pz = std::fma(dir.z, ty, v.z);
            if (pz > 0.0_r && pz < dims.z)
                return ty > 0.0_r ? ty : infinity;
        }

        const real tz = ((std::signbit(dir.z) ? dims.z : 0.0_r) - v.z) * inv_dir.z;
        if (tz > 0.0_r) {
            const real px = std::fma(dir.x, tz, v.x);
            if (px > 0.0_r && px < dims.x) {
                const real py = std::fma(dir.y, tz, v.y);
                return (py > 0.0_r && py < dims.y) ? tz : infinity;
            }
        }
        
        return infinity;
    }
    
    else if constexpr (type == type::Center) {

        // Check whether u is inside the box
        if (std::abs(v.x) <= dims.x && std::abs(v.y) <= dims.y && std::abs(v.z) <= dims.z)
            return 0.0_r;

        const real t1 = (-v.x) * inv_dir.x - dims.x * std::abs(inv_dir.x);
        if (std::abs(std::fma(dir.y, t1, v.y)) <= dims.y && std::abs(std::fma(dir.z, t1, v.z)) <= dims.z)
            return t1 > 0.0_r ? t1 : infinity;

        const real t2 = (-v.y) * inv_dir.y - dims.y * std::abs(inv_dir.y);
        if (std::abs(std::fma(dir.x, t2, v.x)) <= dims.x && std::abs(std::fma(dir.z, t2, v.z)) <= dims.z)
            return t2 > 0.0_r ? t2 : infinity;

        const real t3 = (-v.z) * inv_dir.z - dims.z * std::abs(inv_dir.z);
        return (t3 > 0.0_r && std::abs(std::fma(dir.x, t3, v.x)) <= dims.x && std::abs(std::fma(dir.y, t3, v.y)) <= dims.y) ?
            t3 : infinity;
    }
}