#pragma once

#include "light/ray.hpp"
#include "auxiliary/randomgen.hpp"

// Sampling modes
enum class camera_mode_option : unsigned int {
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
            : mode(std::to_underlying(m)) {}
        
        camera_mode(camera_mode_option m1, camera_mode_option m2)
            : mode(std::to_underlying(m1) | std::to_underlying(m2)) {}

        inline bool uses_dof() const {
            return mode & std::to_underlying(camera_mode_option::Cam_Depth_of_Field);
        }

        inline bool uses_stratified() const {
            return mode & std::to_underlying(camera_mode_option::Cam_Stratified);
        }
};



class camera {

    public:
        struct aa_shift {
            real horiz, vert;
        };
    
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


        /// Helper functions

        inline std::pair<int, int> stratified_shift(const int iteration) const {
            return mode.uses_stratified() ?
                  std::pair { iteration & 0b0011, (iteration & 0b1100) >> 2 }
                : std::pair { 0, 0 };
        }

        inline std::pair<int, int> stratified_shift_pixel(const int i, const int j, const int iteration) const {
            if constexpr (STRATIFIED_ENABLED) {
                const auto [ strat_x, strat_y ] = stratified_shift(iteration);
                return { (i << 2) + strat_x, (j << 2) + strat_y };
            }
            else
                return { i, j };
        }

        inline std::pair<real, real> shift_classic(const int i, const int j, const int iteration) const {
            const auto [ si, sj ] = stratified_shift_pixel(i, j, iteration);
            return { std::fma(di, static_cast<real>(si), mhalf_fovw),
                     std::fma(dj, static_cast<real>(sj), mhalf_fovh) };
        }

        inline std::pair<real, real> shift_normal(const int i, const int j, const int iteration, const aa_shift& shift) const {
            const auto [ si, sj ] = stratified_shift_pixel(i, j, iteration);
            return { std::fma(di, static_cast<real>(si) + shift.horiz, mhalf_fovw),
                     std::fma(dj, static_cast<real>(sj) + shift.vert,  mhalf_fovh) };
        }

        inline rt::vector direction(real ishift, real jshift) const {
            return fma(to_the_right, ishift, fma(to_the_bottom, jshift, direction_scaled)).unit();
        }

    public:
        camera_mode mode;

        camera(const rt::vector& origin, const rt::vector& direction, const rt::vector& to_the_right,
            real fov_w, real fov_h, real dist,
            int width, int height,
            real focal_length = -1.0_r, real aperture = -1.0_r);

        camera(camera&&) noexcept           = default;

        camera(const camera&)               = delete;
        camera& operator=(const camera&)    = delete;
        camera& operator=(camera&&)         = delete;

        /* Returns the ray that goes toward the pixel i,j of the screen */
        ray gen_ray_classic(int i, int j, int iteration) const {
            const auto [ ishift, jshift ] = shift_classic(i, j, iteration);
            return ray(origin, direction(ishift, jshift));
        }

        static inline aa_shift generate_shift(const randomgen& rg) {
            const real radius = rg.random_normal();
            const real angle  = rg.random_angle();
            return {
                .horiz = radius * cos(angle),
                .vert  = radius * sin(angle)
            };
        }

        /* Returns the ray that goes toward the pixel i,j of the screen in average,
           following a normal distribution around to center of the pixel, with given stardard deviation */
        ray gen_ray_normal(int i, int j, int iteration, const aa_shift& shift) const {
            const auto [ ishift, jshift ] = shift_normal(i, j, iteration, shift);
            return ray(origin, direction(ishift, jshift));
        }

        /* Returns the ray that goes toward the pixel i,j of the screen, with depth of field */
        ray gen_ray_dof(int i, int j, const randomgen& rg, int iteration) const;

        ray gen_ray(int i, int j, const randomgen& rg, int iteration, const aa_shift& shift) const {
            return
                mode.uses_dof() ?
                      gen_ray_dof(i, j, rg, iteration)
                    : gen_ray_normal(i, j, iteration, shift);
        }
};