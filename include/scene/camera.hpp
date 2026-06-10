#pragma once

#include "light/ray.hpp"
#include "auxiliary/randomgen.hpp"

// Sampling modes
enum class camera_mode_option {
    Cam_Default         = 0,
    Cam_Depth_of_Field  = 1,
    Cam_Normal_AA       = 2,
    Cam_Stratified      = 4
};

// Replace with constexpr operators and camera_mode_option mode

class camera_mode {
    private:
        unsigned int mode;

    public:
        camera_mode(camera_mode_option m)
            : mode(static_cast<unsigned int>(m)) {}
        
        camera_mode(camera_mode_option m1, camera_mode_option m2)
            : mode(static_cast<unsigned int>(m1) | static_cast<unsigned int>(m2)) {}

        inline bool uses_dof() const {
            return mode & static_cast<unsigned int>(camera_mode_option::Cam_Depth_of_Field);
        }

        inline bool uses_stratified() const {
            return mode & static_cast<unsigned int>(camera_mode_option::Cam_Stratified);
        }
};



class camera {
    
    private:

        /* Position */
        rt::vector origin;

        /* direction is the way forward (z-axis),
           to_the_right is the x-axis, pointed to the right,
           to_the_bottom is the y-axis, it is calculated automatically,
           to_the_bottom = direction ^ to_the_right
        */
        //rt::vector direction;
        rt::vector direction_scaled;
        rt::vector to_the_right;
        rt::vector to_the_bottom;

        /* World space dimensions of the field of view: width and height */
        /* Unused: only the pre-computed mhalf_fovw, mhalf_fovh */
        /* real fov_w;
        real fov_h; */
        
        /* World space distance to the screen */
        //real distance;

        /* Pre-computation: step for i and j in world space */
        real di;
        real dj;
        real mhalf_fovw;
        real mhalf_fovh;

        /* Depth of field */
        real focal_length;
        real aperture;


        std::pair<int, int> stratified_shift(int iteration) const;
        std::pair<real, real> shift_classic(int i, int j, int iteration) const;
        std::pair<real, real> shift_normal(int i, int j, int iteration, const real shift_horiz, const real shift_vert) const;
        rt::vector direction(real ishift, real jshift) const;

    public:
        camera_mode mode;

        camera(const rt::vector& origin, const rt::vector& direction, const rt::vector& to_the_right,
            real fov_w, real fov_h, real dist,
            int width, int height,
            real focal_length = -1.0_r, real aperture = -1.0_r);

        camera(camera&&)                    = default;

        camera(const camera&)               = delete;
        camera& operator=(const camera&)    = delete;
        camera& operator=(camera&&)         = delete;

        /* Returns the ray that goes toward the pixel i,j of the screen */
        ray gen_ray_classic(int i, int j, int iteration) const;

        /* Returns the ray that goes toward the pixel i,j of the screen in average,
           following a normal distribution around to center of the pixel, with given stardard deviation */
        ray gen_ray_normal(int i, int j, const randomgen& rg, int iteration) const;

        /* Returns the ray that goes toward the pixel i,j of the screen, with depth of field */
        ray gen_ray_dof(int i, int j, const randomgen& rg, int iteration) const;

        ray gen_ray(int i, int j, const randomgen& rg, int iteration) const {
            return
                mode.uses_dof() ?
                      gen_ray_dof(i, j, rg, iteration)
                    : gen_ray_normal(i, j, rg, iteration);
        }
};