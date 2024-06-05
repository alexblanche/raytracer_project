#pragma once

#include "vector.hpp"
#include "screen/color.hpp"

using namespace std;

/** 
 * The ray class contains the information of a ray of light:
 * the point of origin, the direction vector,
 * as well as two pre-computed vectors inv_dir = (1/d.x, 1/d.y, 1/d.z)
 * and abs_inv_dir = (abs(inv_d.x), abs(inv_d.y), abs(inv_z.z))
*/

class ray {
    
    protected:
        rt::vector origin;

        rt::vector direction;

        /* Pre-computed values */

        // Inverse of the direction
        rt::vector inv_dir;

        // Absolute values of each component
        rt::vector abs_inv_dir;


    public :

        /* Constructors */
        
        ray (const rt::vector& o, const rt::vector& d);
        
        ray ();
        
        /* Accessors */
        inline const rt::vector& get_origin() const {
            return origin;
        }
        
        inline rt::vector get_direction() const {
            return direction;
        }

        inline rt::vector get_inv_dir() const {
            return inv_dir;
        }

        inline rt::vector get_abs_inv_dir() const {
            return abs_inv_dir;
        }

        /* Mutators */
        inline void set_origin(const rt::vector& o) {
            origin = o;
        }

        void set_direction(const rt::vector& direction);
};

