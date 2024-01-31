#pragma once

#include "vector.hpp"
#include "../../screen/headers/color.hpp"

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
};

