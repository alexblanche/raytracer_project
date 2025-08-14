#include "scene/objects/plane.hpp"

#include "light/vector.hpp"
#include "light/hit.hpp"
#include "scene/material/material.hpp"

#include <cmath>
#include <stdexcept>
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
    
    : object(rt::vector(), material_index) {

    /* Normalization of the normal vector */
    const rt::vector normal_vector(pa, pb, pc);
    normal = normal_vector.unit();
    a = normal.x;
    b = normal.y;
    c = normal.z;
    d = pd / normal_vector.norm();
    
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

    : object(position, material_index) {

    normal = rt::vector(pa, pb, pc).unit();
    a = normal.x;
    b = normal.y;
    c = normal.z;

    d = -(normal | position); // = -aX-bY-cZ if position = (X,Y,Z)
}

// Constructor for textured planes
plane::plane(const real pa, const real pb, const real pc, const rt::vector& position,
    const unsigned int material_index,
    const std::optional<texture_info>& info, const rt::vector& right, const real scale)

    : object(position, material_index, info) {

    normal = rt::vector(pa, pb, pc).unit();
    a = normal.x;
    b = normal.y;
    c = normal.z;

    d = -(normal | position); // = -aX-bY-cZ if position = (X,Y,Z)

    orientation.right_dir = right.unit();
    orientation.down_dir = orientation.right_dir ^ normal;
    orientation.inv_texture_scale = 1.0f / scale;
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
    
    return (pdt * upln < 0.0f) ?
        std::optional<real>(- upln / pdt)
        :
        std::nullopt;
}

hit plane::compute_intersection(ray& r, const real t) const {

    // Intersection point
    //const rt::vector p = r.get_origin() + t * r.get_direction();
    const rt::vector p = fma(r.get_direction(), t, r.get_origin());

    // The normal vector (a, b, c) is assumed to be a unit vector

    const object* pt_obj = this;
    ray* pt_ray = &r;
    return hit(pt_ray, p, get_normal(), pt_obj);
}

/* Returns the barycentric info (tiles according to texture_scale) */
barycentric_info plane::get_barycentric(const rt::vector& p) const {

    const real right_component = (p | orientation.right_dir) * orientation.inv_texture_scale;
    real x_value = fmod(right_component, (real) 1.0f);
    if (x_value < 0) x_value += 1.0f;

    const real down_component = (p | orientation.down_dir) * orientation.inv_texture_scale;
    real y_value = fmod(down_component, (real) 1.0f);
    if (y_value < 0) y_value += 1.0f;

    return barycentric_info(x_value, y_value);
}

/* Normal map vector computation at render time */
rt::vector plane::compute_normal_from_map(const rt::vector& tangent_space_normal, const rt::vector& local_normal) const {

    //return tangent_space_normal.x * right_dir.value() + tangent_space_normal.y * down_dir.value() + tangent_space_normal.z * local_normal;
    return matprod(
        orientation.right_dir, tangent_space_normal.x,
        orientation.down_dir,  tangent_space_normal.y,
        local_normal,          tangent_space_normal.z
    );
}


rt::vector plane::sample(randomgen& /*rg*/) const {
    throw std::runtime_error("Sampling is unavailable for planes");
}


rt::vector plane::sample_visible(randomgen& /*rg*/, const rt::vector& /*pt*/) const {
    throw std::runtime_error("Sampling is unavailable for planes");
}