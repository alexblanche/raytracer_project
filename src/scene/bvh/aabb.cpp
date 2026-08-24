#include "scene/bvh/aabb.hpp"

#include <iostream>

#include "auxiliary/utils.hpp"

real aabb::measure_distance(const ray& r) const {

    const auto& [ u, dir, inv_dir ] = r;
    const auto& [ u_x, u_y, u_z ] = u - position;

    if constexpr (type_ == type::Corner) {

        if ((u_x > 0.0_r && u_x < dims.x) && (u_y > 0.0_r && u_y < dims.y) && (u_z > 0.0_r && u_z < dims.z)) {
            // u is inside the box
            return 0.0_r;
        }

        /* Only one t can reach the box, so we return as soon as one fits */

        const real tx = ((std::signbit(dir.x) ? dims.x : 0.0_r) - u_x) * inv_dir.x;
        const real py = std::fma(dir.y, tx, u_y);
        if (py > 0.0_r && py < dims.y) {
            const real pz = std::fma(dir.z, tx, u_z);
            if (pz > 0.0_r && pz < dims.z)
                return tx > 0.0_r ? tx : infinity;
        }
        
        const real ty = ((std::signbit(dir.y) ? dims.y : 0.0_r) - u_y) * inv_dir.y;
        const real px = std::fma(dir.x, ty, u_x);
        if (px > 0.0_r && px < dims.x) {
            const real pz = std::fma(dir.z, ty, u_z);
            if (pz > 0.0_r && pz < dims.z)
                return ty > 0.0_r ? ty : infinity;
        }

        const real tz = ((std::signbit(dir.z) ? dims.z : 0.0_r) - u_z) * inv_dir.z;
        if (tz > 0.0_r) {
            const real px = std::fma(dir.x, tz, u_x);
            if (px > 0.0_r && px < dims.x) {
                const real py = std::fma(dir.y, tz, u_y);
                return (py > 0.0_r && py < dims.y) ? tz : infinity;
            }
        }
        
        return infinity;
    }
    
    else if constexpr (type_ == type::Center) {

        // Check whether u is inside the box
        if (std::abs(u_x) <= dims.x && std::abs(u_y) <= dims.y && std::abs(u_z) <= dims.z)
            return 0.0_r;

        const real t_x = (-u_x - copysign(dims.x, dir.x)) * inv_dir.x;
        if (std::abs(std::fma(dir.y, t_x, u_y)) <= dims.y && std::abs(std::fma(dir.z, t_x, u_z)) <= dims.z)
            return t_x > 0.0_r ? t_x : infinity;

        const real t_y = (-u_y - copysign(dims.y, dir.y)) * inv_dir.y;
        if (std::abs(std::fma(dir.x, t_y, u_x)) <= dims.x && std::abs(std::fma(dir.z, t_y, u_z)) <= dims.z)
            return t_y > 0.0_r ? t_y : infinity;

        const real t_z = (-u_z - copysign(dims.z, dir.z)) * inv_dir.z;
        return (t_z > 0.0_r
            && std::abs(std::fma(dir.x, t_z, u_x)) <= dims.x && std::abs(std::fma(dir.y, t_z, u_y)) <= dims.y) ?
            t_z : infinity;
    }
}