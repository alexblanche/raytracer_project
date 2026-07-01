#include "scene/bounding/aabb.hpp"

#include "auxiliary/utils.hpp"

real aabb::measure_distance(const ray& r) const {

    const auto& [ u, dir, inv_dir ] = r;
    const rt::vector v = u - corner;
    const rt::vector inside = v * inv_dims;

    if (is_between_zero_and_one(inside.x) && is_between_zero_and_one(inside.y) && is_between_zero_and_one(inside.z)) {
        // u is inside the box
        return 0.0_r;
    }
    
    // Computation of the distance to the three visible faces from u
    const rt::vector sign(std::signbit(dir.x), std::signbit(dir.y), std::signbit(dir.z));
    const rt::vector t = ((dims * sign) - v) * inv_dir;
    const bool is_admissible_t[3] = { is_positive(t.x), is_positive(t.y), is_positive(t.z) };

    constexpr rt::vector FAR_AWAY(infinity, infinity, infinity);
    const rt::vector proj[3] = {
        is_admissible_t[0] ? r.extend(t.x) : FAR_AWAY,
        is_admissible_t[1] ? r.extend(t.y) : FAR_AWAY,
        is_admissible_t[2] ? r.extend(t.z) : FAR_AWAY
    };
    const auto& [ px, py, pz ] = proj;

    const bool intersects[3] = {
        is_admissible_t[0] && is_between_zero_and_one(px.y * inv_dims.y) && is_between_zero_and_one(px.z * inv_dims.z),
        is_admissible_t[1] && is_between_zero_and_one(py.x * inv_dims.x) && is_between_zero_and_one(py.z * inv_dims.z),
        is_admissible_t[2] && is_between_zero_and_one(pz.x * inv_dims.x) && is_between_zero_and_one(pz.y * inv_dims.y)
    };

    const rt::vector dist = {
        intersects[0] ? t.x : infinity,
        intersects[1] ? t.y : infinity,
        intersects[2] ? t.z : infinity
    };

    return std::min(dist.x, std::min(dist.y, dist.z));
}