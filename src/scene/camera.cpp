#include "scene/camera.hpp"

camera::camera(const rt::vector& origin, const rt::vector& direction, const rt::vector& to_the_right,
    const real fov_w, const real fov_h, const real dist,
    const int width, const int height,
    const real focal_length, const real aperture)

    : origin(origin),
      direction_scaled(direction.unit() * dist),
      to_the_right(to_the_right.unit()),
      to_the_bottom((direction ^ to_the_right).unit()),
      di((STRATIFIED_ENABLED ? fov_w * 0.25_r : fov_w) / static_cast<real>(width)),
      dj((STRATIFIED_ENABLED ? fov_h * 0.25_r : fov_h) / static_cast<real>(height)),
      mhalf_fovw(-fov_w / 2.0_r), mhalf_fovh(-fov_h / 2.0_r),
      focal_length(focal_length), aperture(aperture),
      mode(
        ((aperture < 0.0_r) ?
              camera_mode_option::Cam_Normal_AA
            : camera_mode_option::Cam_Depth_of_Field),
        (STRATIFIED_ENABLED ?
              camera_mode_option::Cam_Stratified
            : camera_mode_option::Cam_Default)) {}

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
