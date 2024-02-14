#include "headers/plane.hpp"

#include "../../light/headers/vector.hpp"
#include "../../light/headers/hit.hpp"
#include "../material/headers/material.hpp"

#include<limits>
numeric_limits<double> realpln;
const double infinity = realpln.infinity();

/* Constructors */

/* Default constructor */
plane::plane() : a(0), b(0), c(0), d(0) {}

/* Main constructor */
/* A plane (P) of equation (P): ax + by + cz + d = 0
 defined by 4 doubles a,b,c,d */
/* The normal vector (a, b, c) is a unit vector */
plane::plane(const double pa, const double pb, const double pc, const double pd,
    const unsigned int index, const material& material) {

    object::index = index;
    object::mat = material;

    /* Normalization of the normal vector */
    const rt::vector n = rt::vector(pa, pb, pc).unit();
    a = n.x;
    b = n.y;
    c = n.z;
    d = pd / sqrt(pa*pa + pb*pb + pc*pc);
    
    if (pa != 0) {
        object::position = rt::vector(-d/a, 0, 0);
    }
    else if (pb != 0) {
        object::position = rt::vector(0, -d/b, 0);
    }
    else if (pc != 0) {
        object::position = rt::vector(0, 0, -d/c);
    }
    else {
        object::position = rt::vector(0, 0, 0);
    }
}

/* Constructor of a plane of normal vector (a,b,c) and touching the point v */
plane::plane(const double pa, const double pb, const double pc, const rt::vector& position,
    const unsigned int index, const material& material)
    : object(position, index, material) {
    const rt::vector n = rt::vector(pa, pb, pc).unit();
    a = n.x;
    b = n.y;
    c = n.z;

    d = -(n | position); // = -aX-bY-cZ if position = (X,Y,Z)
}


/* Accessors */

/* The normal is assumed to be a unit vector, ensured at construction */
rt::vector plane::get_normal() const {
    return rt::vector(a, b, c);
}

double plane::get_d() const {
    return d;
}


/* Intersection determination */

double plane::measure_distance(const ray& r) const {

    /* Origin of the ray:    u   = (X, Y, Z)
       Direction of the ray: dir = (x, y, z)
       Normal of the plane:  n   = (a, b, c)
       The normal and the direction are supposed to be unit vectors.
       
       We search t so that u + t.dir belongs to the plane,
       i.e. a(X + tx) + b(Y + ty) + c(Z + tz) + d = 0,
       => t = -(aX + bY + cZ + d) / (ax + by + cz)
    */

    const rt::vector n = get_normal();
    const double pdt = (n | r.get_direction()); // ax + by + cz
    const double upln = (n | r.get_origin()) + d; // aX + bY + cZ + d
    
    // If -upln/pdt > 0, it is our solution t, otherwise the plane is either parallel (pdt == 0) or "behind" the plane (-upln/pdt < 0)
    
    if (pdt * upln < 0) {
        return (- upln / pdt);
    }
    else {
        return infinity;
    }
}

hit plane::compute_intersection(const ray& r, const double t) const {

    // Intersection point
    const rt::vector p = r.get_origin() + t * r.get_direction();

    // The normal vector (a, b, c) is assumed to be a unit vector

    return hit(r, p, get_normal(), get_index());
}
