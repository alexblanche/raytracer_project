#include "headers/sphere.hpp"
#include "../screen/headers/color.hpp"
#include "../light/headers/vector.hpp"
#include <math.h>

#include<limits>


/* Constructors */

sphere::sphere(const rt::vector& center, double radius, const rt::color& color)
    : object(center, color), radius(radius) {}

sphere::sphere() {
    radius = 0;
}

/* Accessors */

rt::vector sphere::get_center() const {
    return position;
}

double sphere::get_radius() const {
    return radius;
}

rt::color sphere::get_color() const {
    return color;
}

/* Intersection determination */

/* Calculates and returns the intersection value t */
double sphere::send(const ray& r) const {
    /*

      We have to solve the equation ||/AC - t/u||^2 = R^2
      The system is equivalent to:
      t^2*||u||^2-2(u|v)t+||v||^2-R^2 = 0
      Delta = 4((u|v)^2-||u||^2*(||v||^2-R^2))

      where ||u|| = 1
      and v = AC = (Cx-Ax, Cy-Ay, Cz-Az)

    */
    rt::vector v = get_center() - r.get_origin();
    rt::vector u = r.get_direction().unit();

    double nv = v.norm();
    double uv = (u|v);

    double A = uv * uv + radius * radius - nv * nv;
    
    // Delta = 4A
    numeric_limits<double> real;
    const double infinity = real.infinity();

    if (A > 0) {
        double t1 = uv - sqrt(A);
        //double t2 = uv + sqrt(A);

        if (t1>0) { // t2>0 because t2>t1
            return t1; // = min(t1,t2)
        }
        /*
        else if (t2>0) {
            return t2;
        }
        */
        else {
            return infinity;
        }
    }
    else {
        return infinity;
    }
}

/* Returns the hit corresponding with the given intersection value t */
hit sphere::intersect(const ray& r, double t) const {

    // Intersection point
    rt::vector p = r.get_origin() + t * (r.get_direction().unit());
    
    // Normal vector
    rt::vector n = (get_center() - p).unit();

    return (hit(r, p, n, color));
}

