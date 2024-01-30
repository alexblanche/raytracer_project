#include "headers/sphere.hpp"
#include "../screen/headers/color.hpp"
#include "../light/headers/vector.hpp"
#include <math.h>

#include<limits>


/* Constructors */
sphere::sphere(const rt::vector& c, double r, const rt::color& co)
    : center(c), radius(r), col(co) {}

sphere::sphere() {
    center = rt::vector();
    radius = 0;
    col = rt::color::WHITE;
}

/* Accessors */
rt::vector sphere::get_center() const {
    return center;
}

double sphere::get_radius() const {
    return radius;
}

rt::color sphere::get_color() const {
    return col;
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
    rt::vector C = center;
    rt::vector O = r.get_origin();
    rt::vector v = C-O;
    rt::vector u = (r.get_direction()).unit();

    double nv = v.norm();
    double uv = (u|v);
    double R = radius;

    double A = uv*uv + R*R - nv*nv;
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

/* Calculates the intersection value t and returns the corresponding hit */
/*
hit sphere::intersect(const ray& r) const {
    rt::vector C = center;
    rt::vector O = r.get_origin();
    rt::vector v = C-O;
    rt::vector u = (r.get_direction()).unit();

    double nv = v.norm();
    double uv = (u|v);
    double R = radius;

    double A = (uv*uv +R*R - nv*nv);
    // Delta = 4A

    if (A > 0) {
        double t1 = uv - sqrt(A);
        //double t2 = uv + sqrt(A);

        if (t1>0) { // t2>0 because t2>t1
            double tf=t1; // = min(t1,t2)
            rt::vector p_inter = r.get_origin()+tf*r.get_direction();
            return hit(r,p_inter,(center-p_inter).unit(),col);
        }
        
        // else if (t2>0) {
        //    double tf=t2;
        //    rt::vector p_inter=r.get_origin()+tf*r.get_direction();
        //    return hit(r,p_inter,(center-p_inter).unit(),col);
        //}
        
        else {
            return hit();
        }
    }
    else {
        return hit();
    }
}
*/

/* Returns the hit corresponding with the given intersection value t */
hit sphere::intersect(const ray& r, double t) const {

    // Intersection point
    rt::vector p = r.get_origin() + t * (r.get_direction()).unit();
    
    // Normal vector
    rt::vector n = (center-p).unit();

    return (hit(r, p, n, col));
}

