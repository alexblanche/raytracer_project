#pragma once

#include "other/sky/sky_screen.hpp"
#include "other/sky/sky_vector.hpp"

struct render_parameters {
    sky::screen scr;
    SDL_Texture* txt;
    mutable char* texture_pixels;
    char* orig_pixels;
    mutable int texture_pitch;
    int img_width;
    int img_height;
    sky::vector scaled_x_axis;
    sky::vector scaled_y_axis;
    sky::vector axes_center;
    float img_scale_x;
    float img_scale_y;

    render_parameters()
        : scr(width, height) {}
};

void render(const render_parameters& param);

struct segment_loop_parameters {
    float theta;
    float phi;
    sky::vector cartesian;
    int lim_int;
    float x;
    float y;
    int index;
    int index_loop;
};