#include "light/vector.hpp"
#include "light/ray.hpp"

#include "auxiliary/randomgen.hpp"
#include "scene/camera.hpp"

camera::camera(const rt::vector& origin, const rt::vector& direction, const rt::vector& to_the_right,
    const real fov_w, const real fov_h, const real dist,
    const int width, const int height)

    : origin(origin), direction(direction.unit()), to_the_right(to_the_right.unit()),
      to_the_bottom((direction ^ to_the_right).unit()), distance(dist),
      di(fov_w / ((real) width)), dj(fov_h / ((real) height)),
      mhalf_fovw(-fov_w/2), mhalf_fovh(-fov_h/2),
      focal_length(-1), aperture(-1),
      mode(camera_mode_option::Cam_Normal_AA, camera_mode_option::Cam_Stratified) {}

camera::camera(const rt::vector& origin, const rt::vector& direction, const rt::vector& to_the_right,
    const real fov_w, const real fov_h, const real dist,
    const int width, const int height,
    const real focal_length, const real aperture)

    : origin(origin), direction(direction.unit()), to_the_right(to_the_right.unit()),
      to_the_bottom((direction ^ to_the_right).unit()), distance(dist),
      di(fov_w / ((real) width)), dj(fov_h / ((real) height)),
      mhalf_fovw(-fov_w/2), mhalf_fovh(-fov_h/2),
      focal_length(focal_length), aperture(aperture),
      mode(camera_mode_option::Cam_Depth_of_Field, camera_mode_option::Cam_Stratified) {}


/* Returns the ray that goes toward the pixel i,j of the screen */
ray camera::gen_ray_classic(const int i, const int j, const unsigned int iteration) const {

    const real ishift = mhalf_fovw + (static_cast<real>(i) + (mode.uses_stratified() ? 0.25f * ( iteration &  3)       : 0.0f)) * di;
    const real jshift = mhalf_fovh + (static_cast<real>(j) + (mode.uses_stratified() ? 0.25f * ((iteration & 15) >> 2) : 0.0f)) * dj;
    const rt::vector dir =
        (matprod(
            to_the_right,  ishift,
            to_the_bottom, jshift,
            direction,     distance)
        ).unit();
    return ray(origin, dir);
}

/* Returns the ray that goes toward the pixel i,j of the screen in average,
   following a normal distribution around to center of the pixel, with given stardard deviation */
ray camera::gen_ray_normal(const int i, const int j, const randomgen& rg, const unsigned int iteration) const {

    const auto [ shift_horiz, shift_vert ] = rg.random_pair_normal(); //rg.random_pair_normal(0.0f, std_dev);
    const real ishift = mhalf_fovw + (static_cast<real>(i) + shift_horiz  + (mode.uses_stratified() ? 0.25f * ( iteration &  3)       : 0.0f)) * di;
    const real jshift = mhalf_fovh + (static_cast<real>(j) + shift_vert + (mode.uses_stratified() ? 0.25f * ((iteration & 15) >> 2) : 0.0f)) * dj;
    const rt::vector dir =
        (matprod(
            to_the_right,  ishift,
            to_the_bottom, jshift,
            direction,     distance))
        .unit();
    return ray(origin, dir);
}

/* Returns the ray that goes toward the pixel i,j of the screen, with depth of field */
ray camera::gen_ray_dof(const int i, const int j, const randomgen& rg, const unsigned int iteration) const {

    const real ishift = mhalf_fovw + (static_cast<real>(i) + (mode.uses_stratified() ? 0.25f * ( iteration &  3)       : 0.0f)) * di;
    const real jshift = mhalf_fovh + (static_cast<real>(j) + (mode.uses_stratified() ? 0.25f * ((iteration & 15) >> 2) : 0.0f)) * dj;
    const rt::vector focus_point =
        focal_length *
        ((matprod(
            to_the_right,  ishift,
            to_the_bottom, jshift,
            direction,     distance))
        .unit());
        
    const real r = rg.random_ratio();
    const real phi = rg.random_angle();
    const real apr_r = aperture * std::sqrt(r);
    const rt::vector starting_point =
          fma(to_the_right, apr_r * cos(phi),
            (apr_r * sin(phi)) * to_the_bottom);
    const rt::vector dir = (focus_point - starting_point).unit();
    return ray(origin + starting_point, dir);
}
