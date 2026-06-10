#include "scene/camera.hpp"

camera::camera(const rt::vector& origin, const rt::vector& direction, const rt::vector& to_the_right,
    const real fov_w, const real fov_h, const real dist,
    const int width, const int height,
    const real focal_length, const real aperture)

    : origin(origin), direction_scaled(direction.unit() * dist), to_the_right(to_the_right.unit()),
      to_the_bottom((direction ^ to_the_right).unit()), //distance(dist),
      di(fov_w * 0.25_r / static_cast<real>(width)), dj(fov_h * 0.25_r / static_cast<real>(height)),
      mhalf_fovw(-fov_w / 2.0_r), mhalf_fovh(-fov_h / 2.0_r),
      focal_length(focal_length), aperture(aperture),
      mode((aperture < 0.0_r) ?
                  camera_mode_option::Cam_Normal_AA
                : camera_mode_option::Cam_Depth_of_Field,
        camera_mode_option::Cam_Stratified) {}

inline std::pair<int, int> camera::stratified_shift(const int iteration) const {
    return mode.uses_stratified() ?
          std::pair {
             (iteration & 0b0011),
            ((iteration & 0b1100) >> 2) }
        : std::pair { 0, 0 };
}

#include <iostream>

inline std::pair<real, real> camera::shift_classic(const int i, const int j, const int iteration) const {

    const auto [ strat_x, strat_y ] = stratified_shift(iteration);
    return {
        std::fma(di, static_cast<real>((i << 2) + strat_x), mhalf_fovw),
        std::fma(dj, static_cast<real>((j << 2) + strat_y), mhalf_fovh)
    };
}

inline std::pair<real, real> camera::shift_normal(const int i, const int j, const int iteration, const real shift_horiz, const real shift_vert) const {
    
    const auto [ strat_x, strat_y ] = stratified_shift(iteration);
    return {
        std::fma(di, static_cast<real>((i << 2) + strat_x) + shift_horiz, mhalf_fovw),
        std::fma(dj, static_cast<real>((j << 2) + strat_y) + shift_vert,  mhalf_fovh)
    };
}

inline rt::vector camera::direction(const real ishift, const real jshift) const {
    return
        fma(to_the_right,  ishift,
		fma(to_the_bottom, jshift,
            direction_scaled)
        ).unit();
}

/* Returns the ray that goes toward the pixel i,j of the screen */
ray camera::gen_ray_classic(const int i, const int j, const int iteration) const {

    const auto [ ishift, jshift ] = shift_classic(i, j, iteration);
    return ray(origin, direction(ishift, jshift));
}

/* Returns the ray that goes toward the pixel i,j of the screen in average,
   following a normal distribution around to center of the pixel, with given stardard deviation */
ray camera::gen_ray_normal(const int i, const int j, const randomgen& rg, const int iteration) const {

    const auto [ shift_horiz, shift_vert ] = rg.random_pair_normal();
    const auto [ ishift, jshift ] = shift_normal(i, j, iteration, shift_horiz, shift_vert);
    return ray(origin, direction(ishift, jshift));
}

/* Returns the ray that goes toward the pixel i,j of the screen, with depth of field */
ray camera::gen_ray_dof(const int i, const int j, const randomgen& rg, const int iteration) const {

    const auto [ ishift, jshift ] = shift_classic(i, j, iteration);
    const rt::vector focus_point = focal_length * direction(ishift, jshift);
    const real r = rg.random_ratio();
    const real phi = rg.random_angle();
    const real apr_r = aperture * std::sqrt(r);
    const rt::vector starting_point =
          fma(to_the_right,    apr_r * cos(phi),
              to_the_bottom * (apr_r * sin(phi)));
    const rt::vector dir = (focus_point - starting_point).unit();
    return ray(origin + starting_point, dir);
}
