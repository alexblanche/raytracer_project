#include "light/vector.hpp"
#include "light/ray.hpp"

#include "auxiliary/randomgen.hpp"
#include "scene/camera.hpp"

#define TWOPI 6.2831853071795862f

camera::camera() {}

camera::camera(const rt::vector& origin, const rt::vector& direction, const rt::vector& to_the_right,
    const real& fov_w, const real& fov_h, const real& dist,
    const int width, const int height)

    : origin(origin), direction(direction.unit()), to_the_right(to_the_right.unit()),
      to_the_bottom((direction ^ to_the_right).unit()), distance(dist),
      di(fov_w / ((real) width)), dj(fov_h / ((real) height)),
      mhalf_fovw(-fov_w/2), mhalf_fovh(-fov_h/2),
      focal_length(-1), aperture(-1), depth_of_field_enabled(false) {}

camera::camera(const rt::vector& origin, const rt::vector& direction, const rt::vector& to_the_right,
    const real& fov_w, const real& fov_h, const real& dist,
    const int width, const int height,
    const real& focal_length, const real& aperture)

    : origin(origin), direction(direction.unit()), to_the_right(to_the_right.unit()),
      to_the_bottom((direction ^ to_the_right).unit()), distance(dist),
      di(fov_w / ((real) width)), dj(fov_h / ((real) height)),
      mhalf_fovw(-fov_w/2), mhalf_fovh(-fov_h/2),
      focal_length(focal_length), aperture(aperture), depth_of_field_enabled(true) {}


/* Returns the ray that goes toward the pixel i,j of the screen */
ray camera::gen_ray(const int i, const int j) const {
    rt::vector dir = (mhalf_fovw + ((real) i) * di) * to_the_right
        + (mhalf_fovh + ((real) j) * dj) * to_the_bottom
        + distance * direction;
    return ray(origin, dir.unit());
}

/* Returns the ray that goes toward the pixel i,j of the screen in average,
   following a normal distribution around to center of the pixel, with given stardard deviation */
ray camera::gen_ray_normal(const int i, const int j, const real& std_dev, randomgen& rg) const {
    std::pair<real, real> shift = rg.random_pair_normal(0.0f, std_dev);
    rt::vector dir = (mhalf_fovw + (((real) i) + shift.first) * di) * to_the_right
        + (mhalf_fovh + (((real) j) + shift.second) * dj) * to_the_bottom
        + distance * direction;
    return ray(origin, dir.unit());
}

/* Returns the ray that goes toward the pixel i,j of the screen, with depth of field */
ray camera::gen_ray_dof(const int i, const int j, randomgen& rg) const {
    rt::vector focus_point =
        focal_length *
        (((mhalf_fovw + ((real) i) * di) * to_the_right
          + (mhalf_fovh + ((real) j) * dj) * to_the_bottom
          + distance * direction).unit()
        );
    const real r = rg.random_real(1.0f);
    const real phi = rg.random_real(TWOPI);
    const real apr_r = aperture * sqrt(r);
    const rt::vector starting_point =
          (apr_r * cos(phi) * to_the_right)
        + (apr_r * sin(phi) * to_the_bottom);
    const rt::vector dir = (focus_point - starting_point).unit();
    return ray(origin + starting_point, dir);
}
