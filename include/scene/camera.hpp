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
        double fov_w;
        double fov_h;
        /* World space distance to the screen */
        double distance;

        /* Pre-computation: step for i and j in world space */
        double di;
        double dj;
        double mhalf_fovw;
        double mhalf_fovh;


    public:
        /* Constructors */
        camera();

        camera(const rt::vector& origin, const rt::vector& direction, const rt::vector& to_the_right,
            const double& fov_w, const double& fov_h, const double& dist,
            const int width, const int height);

        /* Returns the ray that goes toward the pixel i,j of the screen */
        ray gen_ray(const int i, const int j) const;

        /* Returns the ray that goes toward the pixel i,j of the screen in average,
           following a normal distribution around to center of the pixel, with given stardard deviation */
        ray gen_ray_normal(const int i, const int j, const double& std_dev, randomgen& rg) const;
};