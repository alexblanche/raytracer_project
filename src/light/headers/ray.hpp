#pragma once

#include "vector.hpp"
#include "../../screen/headers/color.hpp"
#include "source.hpp"
#include "../../objects/headers/object.hpp"

using namespace std;

/** 
 * The ray class contains the information of a ray of light:
 * the point of origin, the direction vector and the color.
*/


class ray {
    protected:
        rt::vector origin;
        rt::vector direction;
        rt::color color;

  public :
      /* Constructors */

      ray (const rt::vector& o,const rt::vector& d, const rt::color& c);
      
      ray (const rt::vector& o, const rt::vector& d);
      
      ray ();
      
      /* Accessors */
      rt::vector get_origin() const;
      
      rt::vector get_direction() const;
      
      rt::color get_color() const;


      /* Tracing the ray */

      /* Test raycasting function:
        Returns only the color of the surface hit by the ray */
      rt::color cast_ray(const vector<object>& obj_set) const;

      /* Ray tracing function: computes the hit of the given ray on the closest object,
        then applies all the light from all the sources (blocked by the other objects),
        and returns the resulting color. */
      rt::color ray::launch_ray(const vector<object>& obj_set, const vector<source>& light_set) const;
};
