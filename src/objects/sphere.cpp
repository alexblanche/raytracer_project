#include "headers/sphere.hpp"
#include "../screen/headers/color.hpp"
#include "../light/headers/vector.hpp"
#include <math.h>

#include<limits>


// constructors
sphere::sphere(rt::vector c, double r, rt::color co) : center(c), radius(r), col(co) {};

sphere::sphere() {
  center = rt::vector();
  radius = 0;
  col = rt::color::WHITE;
}

// accessors
rt::vector sphere::get_center() {
  return center;
}

double sphere::get_radius() {
  return radius;
}

rt::color sphere::get_color() {
  return col;
}

double sphere::send(ray r) {
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
  };
}

hit sphere::intersect(ray r) {
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
        /*
        else if (t2>0) {
            double tf=t2;
            rt::vector p_inter=r.get_origin()+tf*r.get_direction();
            return hit(r,p_inter,(center-p_inter).unit(),col);
        }
        */
        else {
            return hit();
        }
    }
    else {
        return hit();
    }
}


hit sphere::intersect2(ray r, double t) {
  
    // r is the generator ray
    rt::vector p; // intersection point
    rt::vector n; // normal vector
    rt::color c; // color of the sphere after application of the color of the ray

    rt::color r_col;

    p = r.get_origin() + t * (r.get_direction()).unit();
    n = (center-p).unit();

    //r_col = r.get_color(); // color of the ray

    /*c = rt::color((r_col.get_red() + col.get_red())/2 ,
                  (r_col.get_green() + col.get_green())/2 ,
                  (r_col.get_blue() + col.get_blue())/2);
    // Mean between the color of the ray and the sphere */

    //c = r_col;
    c = col;

    // WE DON'T CARE ABOUT THE COLOR OF THE RAY

    return (hit(r,p,n,c));
}
// Function that returns the hit of intersect without re-calculating the solutions of the equation

