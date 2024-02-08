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

    public :
        /* Constructors */
        
        ray (const rt::vector& o, const rt::vector& d);
        
        ray ();
        
        /* Accessors */
        rt::vector get_origin() const;
        
        rt::vector get_direction() const;

        /* Mutators */
        void set_origin(const rt::vector& origin);

        void set_direction(const rt::vector& direction);
};

