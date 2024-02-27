#pragma once

#include "vector.hpp"
#include "../../screen/headers/color.hpp"

using namespace std;

/** 
 * The ray class contains the information of a ray of light:
 * the point of origin, the direction vector and the color.
 * origin_index is the index of the object (in object::index) the ray originates from, or -1 if it originates from the camera.
*/

class ray {
    protected:
        rt::vector origin;
        rt::vector direction;
        //unsigned int origin_index;

        /* Pre-computed values */

        // Inverse of the direction
        rt::vector inv_dir;
        // Absolute values of each component
        rt::vector abs_inv_dir;


    public :
        // static vector<unsigned int> obj_comp_cpt;

        /* Constructors */
        
        ray (const rt::vector& o, const rt::vector& d);
        
        ray ();
        
        /* Accessors */
        rt::vector get_origin() const {
            return origin;
        }
        
        rt::vector get_direction() const {
            return direction;
        }

        rt::vector get_inv_dir() const {
            return inv_dir;
        }

        rt::vector get_abs_inv_dir() const {
            return abs_inv_dir;
        }

        //unsigned int get_origin_index() const;

        /* Mutators */
        void set_origin(const rt::vector& o) {
            origin = o;
        }

        void set_direction(const rt::vector& direction);

        //void set_origin_index(const unsigned int index);
};

