#include "light/vector.hpp"
#include "light/ray.hpp"

#include "scene/camera.hpp"

camera::camera() {}

camera::camera(const rt::vector& origin, const rt::vector& direction, const rt::vector& to_the_right,
    const double& fov_w, const double& fov_h, const double& dist,
    const int width, const int height)

    : origin(origin), direction(direction.unit()), to_the_right(to_the_right.unit()),
      to_the_bottom((direction ^ to_the_right).unit()),
      fov_w(fov_w), fov_h(fov_h), distance(dist),
      di(fov_w / ((double) width)), dj(fov_h / ((double) height)),
      mhalf_fovw(-fov_w/2), mhalf_fovh(-fov_h/2) {}


/* Returns the ray that goes toward the pixel i,j of the screen */
ray camera::gen_ray(const int i, const int j) const {
    rt::vector dir = (mhalf_fovw + ((double) i) * di) * to_the_right
        + (mhalf_fovh + ((double) j) * dj) * to_the_bottom
        + distance * direction;
    return ray(origin, dir.unit());
}
