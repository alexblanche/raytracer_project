#include "headers/plane.hpp"

#include "../screen/headers/color.hpp"
#include "../light/headers/vector.hpp"
#include "../light/headers/hit.hpp"

#include<limits>

/* Constructors */

/* Default constructor */
plane::plane() {
    a = 0;
    b = 0;
    c = 0;
    d = 0;
    col = rt::color::WHITE;
}

/* Main constructor */
/* A plane (P) of equation (P): ax + by + cz + d = 0
 defined by 4 doubles a,b,c,d */
plane::plane(double ca, double cb, double cc, double cd, const rt::color& ccol)
    : a(ca), b(cb), c(cc), d(cd), col(ccol) {}

/* Constructor of a plane of normal vector (a,b,c) and touching the point v */
plane::plane(double ca, double cb, double cc, const rt::vector& v, const rt::color& ccol) {
    a = ca;
    b = cb;
    c = cc;
    col = ccol;

    d = -((rt::vector(ca,cb,cc))|v); // = -aX-bY-cZ if v = (X,Y,Z)
}

/* Accessors */

rt::vector plane::get_normal() const {
    return (rt::vector(a,b,c));
}

double plane::get_d() const {
    return d;
}

rt::color plane::get_color() const {
    return col;
}


/* Intersection determination */

double plane::send(const ray& r) const {

    rt::vector A = r.get_origin(); //A = (xA,yA,zA)
    rt::vector v = (r.get_direction()).unit(); // v = (x,y,z)
    // We search t so that
    // a(xA+t*x)+b(yA+t*y)+c(zA+t*z)+d = 0
    double t;
    numeric_limits<double> real;
    const double infinity = real.infinity();

    rt::vector n(a,b,c);
    n = n.unit();
    double pdt = (n|v);

    if (pdt == 0) { // v is parallel to the plane, so there is no intersection
        return infinity;
    }
    else {
        t = -((n|A)+d) / pdt;
        // t = -(axA+byA+czA+d)/(ax+by+cz)

        if (t <= 0) { // the plane is "behind" the ray, so there is no intersection with the plane
            return infinity;
        }
        else {
            return t;
            // the intersection is:
            // (xA,yA,zA) + t(x,y,z)
        }
    }
}

hit plane::intersect(const ray& r, double t) const {
    rt::vector p = r.get_origin() + t * (r.get_direction()).unit();
    rt::vector n(a,b,c);
    return (hit(r,p,n.unit(),col));
}
