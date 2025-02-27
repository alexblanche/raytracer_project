#include "scene/objects/plane.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"

#include <cmath>

#include <optional>

/* Constructors */

/* Default constructor */
plane::plane() : a(0), b(0), c(0), d(0) {}

/* Main constructor */
/* A plane (P) of equation (P): ax + by + cz + d = 0
 defined by 4 reals a,b,c,d */
/* The normal vector (a, b, c) is a unit vector */
plane::plane(const real pa, const real pb, const real pc, const real pd,
    const unsigned int material_index)
    
    : object(rt::vector(), material_index), right_dir(std::nullopt), down_dir(std::nullopt), inv_texture_scale(1.0f) {

    /* Normalization of the normal vector */
    normal = rt::vector(pa, pb, pc).unit();
    a = normal.x;
    b = normal.y;
    c = normal.z;
    d = pd / sqrt(pa*pa + pb*pb + pc*pc);
    
    if (pa != 0) {
        position = rt::vector(-d/a, 0, 0);
    }
    else if (pb != 0) {
        position = rt::vector(0, -d/b, 0);
    }
    else if (pc != 0) {
        position = rt::vector(0, 0, -d/c);
    }
    else {
        position = rt::vector(0, 0, 0);
    }
}

/* Constructor of a plane of normal vector (a,b,c) and touching the point v */
plane::plane(const real pa, const real pb, const real pc, const rt::vector& position,
    const unsigned int material_index)

    : object(position, material_index), right_dir(std::nullopt), down_dir(std::nullopt), inv_texture_scale(1.0f) {

    normal = rt::vector(pa, pb, pc).unit();
    a = normal.x;
    b = normal.y;
    c = normal.z;

    d = -(normal | position); // = -aX-bY-cZ if position = (X,Y,Z)
}

plane::plane(const real pa, const real pb, const real pc, const rt::vector& position,
    const unsigned int material_index,
    const rt::vector& right, const real scale)

    : object(position, material_index), right_dir(right.unit()), inv_texture_scale(1.0 / scale) {

    normal = rt::vector(pa, pb, pc).unit();
    a = normal.x;
    b = normal.y;
    c = normal.z;

    d = -(normal | position); // = -aX-bY-cZ if position = (X,Y,Z)

    down_dir = right_dir.value() ^ normal;
}

/* Intersection determination */

std::optional<real> plane::measure_distance(const ray& r) const {

    /* Origin of the ray:    u   = (X, Y, Z)
       Direction of the ray: dir = (x, y, z)
       Normal of the plane:  n   = (a, b, c)
       The normal and the direction are supposed to be unit vectors.
       
       We search t so that u + t.dir belongs to the plane,
       i.e. a(X + tx) + b(Y + ty) + c(Z + tz) + d = 0,
       => t = -(aX + bY + cZ + d) / (ax + by + cz)
    */

    const rt::vector& n = get_normal();
    const real pdt = (n | r.get_direction()); // ax + by + cz
    const real upln = (n | r.get_origin()) + d; // aX + bY + cZ + d
    
    // If -upln/pdt > 0, it is our solution t, otherwise the plane is either parallel (pdt == 0) or "behind" the plane (-upln/pdt < 0)
    
    if (pdt * upln < 0.0f) {
        return (- upln / pdt);
    }
    else {
        return std::nullopt;
    }
}

hit plane::compute_intersection(ray& r, const real t) const {

    // Intersection point
    const rt::vector p = r.get_origin() + t * r.get_direction();

    // The normal vector (a, b, c) is assumed to be a unit vector

    const object* pt_obj = this;
    ray* pt_ray = &r;
    return hit(pt_ray, p, get_normal(), pt_obj);
}

/* Returns the barycentric info (tiles according to texture_scale) */
barycentric_info plane::get_barycentric(const rt::vector& p) const {

    const real right_component = (p | right_dir.value()) * inv_texture_scale;
    real x_value = fmod(right_component, (real) 1.0f);
    if (x_value < 0) x_value += 1.0f;

    const real down_component = (p | down_dir.value()) * inv_texture_scale;
    real y_value = fmod(down_component, (real) 1.0f);
    if (y_value < 0) y_value += 1.0f;

    return barycentric_info(x_value, y_value);
}