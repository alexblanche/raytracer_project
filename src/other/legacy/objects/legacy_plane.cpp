#include "other/legacy/objects/legacy_plane.hpp"

/* Constructors */

/* Default constructor */
plane::plane() : normal(0, 1, 0), d(0) {}

/* Main constructor */
/* A plane (P) of equation (P): ax + by + cz + d = 0
 defined by 4 reals a,b,c,d */
/* The normal vector (a, b, c) is a unit vector */
plane::plane(const real pa, const real pb, const real pc, const real pd,
    const rt::color& col)
    
    : object(rt::vector(0, 0, 0), col) {

    /* Normalization of the normal vector */
    const rt::vector n(pa, pb, pc);
    const real norm = n.norm();
    normal = n / norm;
    const auto [ a, b, c ] = normal;
    d = pd / norm;
    
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
    const rt::color& col)

    : object(position, col) {

    normal = rt::vector(pa, pb, pc).unit();

    d = - (normal | position); // = -aX-bY-cZ if position = (X,Y,Z)
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

    const real pdt  = (normal | r.direction);  // ax + by + cz
    const real upln = (normal | r.origin) + d; // aX + bY + cZ + d
    
    // If -upln/pdt > 0, it is our solution t, otherwise the plane is either parallel (pdt == 0) or "behind" the plane (-upln/pdt < 0)
    
    return (pdt * upln < 0.0f) ?
        std::optional(- upln / pdt) : std::nullopt;
}

hit plane::compute_intersection(ray& r, const real t) const {

    // Intersection point
    const rt::vector p = fma(r.direction, t, r.origin);

    // The normal vector (a, b, c) is assumed to be a unit vector

    const object* pt_obj = this;

    return hit(p, normal, pt_obj);
}
