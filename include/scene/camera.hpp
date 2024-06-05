#pragma once

#include "light/vector.hpp"
#include "light/ray.hpp"

class camera {
    
    private:

        /* Position */
        rt::vector origin;

        /* direction is the way forward (z-axis),
           to_the_right is the x-axis, pointed to the right,
           to_the_bottom is the y-axis, it is calculated automatically,
           to_the_bottom = direction ^ to_the_right
        */
        rt::vector direction;
        rt::vector to_the_right;
        rt::vector to_the_bottom;

        /* World space dimensions of the field of view: width and height */
        /* Unused: only the pre-computed mhalf_fovw, mhalf_fovh */
        /* real fov_w;
        real fov_h; */
        
        /* World space distance to the screen */
        real distance;

        /* Pre-computation: step for i and j in world space */
        real di;
        real dj;
        real mhalf_fovw;
        real mhalf_fovh;

        /* Depth of field */
        real focal_length;
        real aperture;


    public:
        bool depth_of_field_enabled;

        /* Constructors */
        camera();

        camera(const rt::vector& origin, const rt::vector& direction, const rt::vector& to_the_right,
            const real& fov_w, const real& fov_h, const real& dist,
            const int width, const int height);

        camera(const rt::vector& origin, const rt::vector& direction, const rt::vector& to_the_right,
            const real& fov_w, const real& fov_h, const real& dist,
            const int width, const int height,
            const real& focal_length, const real& aperture);

        /* Returns the ray that goes toward the pixel i,j of the screen */
        ray gen_ray(const int i, const int j) const;

        /* Returns the ray that goes toward the pixel i,j of the screen in average,
           following a normal distribution around to center of the pixel, with given stardard deviation */
        ray gen_ray_normal(const int i, const int j, const real& std_dev, randomgen& rg) const;

        /* Returns the ray that goes toward the pixel i,j of the screen, with depth of field */
        ray gen_ray_dof(const int i, const int j, randomgen& rg) const;
};