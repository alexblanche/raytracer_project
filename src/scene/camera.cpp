#include "light/vector.hpp"
#include "light/ray.hpp"

#include "auxiliary/randomgen.hpp"
#include "scene/camera.hpp"

#define TWOPI 6.2831853071795862f

camera::camera() {}

camera::camera(const rt::vector& origin, const rt::vector& direction, const rt::vector& to_the_right,
    const real fov_w, const real fov_h, const real dist,
    const int width, const int height)

    : origin(origin), direction(direction.unit()), to_the_right(to_the_right.unit()),
      to_the_bottom((direction ^ to_the_right).unit()), distance(dist),
      di(fov_w / ((real) width)), dj(fov_h / ((real) height)),
      mhalf_fovw(-fov_w/2), mhalf_fovh(-fov_h/2),
      focal_length(-1), aperture(-1), mode(CAM_NORMAL_AA | CAM_STRATIFIED) {}

camera::camera(const rt::vector& origin, const rt::vector& direction, const rt::vector& to_the_right,
    const real fov_w, const real fov_h, const real dist,
    const int width, const int height,
    const real focal_length, const real aperture)

    : origin(origin), direction(direction.unit()), to_the_right(to_the_right.unit()),
      to_the_bottom((direction ^ to_the_right).unit()), distance(dist),
      di(fov_w / ((real) width)), dj(fov_h / ((real) height)),
      mhalf_fovw(-fov_w/2), mhalf_fovh(-fov_h/2),
      focal_length(focal_length), aperture(aperture), mode(CAM_DEPTH_OF_FIELD | CAM_STRATIFIED) {}


/* Returns the ray that goes toward the pixel i,j of the screen */
ray camera::gen_ray_classic(const int i, const int j, const unsigned int iteration) const {

    const real ishift = mhalf_fovw + (static_cast<real>(i) + (mode & CAM_STRATIFIED ? 0.25f * ( iteration &  3)       : 0.0f)) * di;
    const real jshift = mhalf_fovh + (static_cast<real>(j) + (mode & CAM_STRATIFIED ? 0.25f * ((iteration & 15) >> 2) : 0.0f)) * dj;
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
ray camera::gen_ray_normal(const int i, const int j, randomgen& rg, const unsigned int iteration) const {

    std::pair<real, real> shift = rg.random_pair_normal(); //rg.random_pair_normal(0.0f, std_dev);
    const real ishift = mhalf_fovw + (static_cast<real>(i) + shift.first  + (mode & CAM_STRATIFIED ? 0.25f * ( iteration &  3)       : 0.0f)) * di;
    const real jshift = mhalf_fovh + (static_cast<real>(j) + shift.second + (mode & CAM_STRATIFIED ? 0.25f * ((iteration & 15) >> 2) : 0.0f)) * dj;
    const rt::vector dir =
        (matprod(
            to_the_right,  ishift,
            to_the_bottom, jshift,
            direction,     distance))
        .unit();
    return ray(origin, dir);
}

/* Returns the ray that goes toward the pixel i,j of the screen, with depth of field */
ray camera::gen_ray_dof(const int i, const int j, randomgen& rg, const unsigned int iteration) const {

    const real ishift = mhalf_fovw + (static_cast<real>(i) + (mode & CAM_STRATIFIED ? 0.25f * ( iteration &  3)       : 0.0f)) * di;
    const real jshift = mhalf_fovh + (static_cast<real>(j) + (mode & CAM_STRATIFIED ? 0.25f * ((iteration & 15) >> 2) : 0.0f)) * dj;
    const rt::vector focus_point =
        focal_length *
        (matprod(
            to_the_right,  ishift,
            to_the_bottom, jshift,
            direction,     distance))
        .unit();
        
    const real r = rg.random_ratio();
    const real phi = rg.random_angle();
    const real apr_r = aperture * sqrt(r);
    const rt::vector starting_point =
          fma(to_the_right, apr_r * cos(phi),
            (apr_r * sin(phi)) * to_the_bottom);
    const rt::vector dir = (focus_point - starting_point).unit();
    return ray(origin + starting_point, dir);
}
