#include "sky/screen.hpp"
#include "sky/vector.hpp"

void render(SDL_Texture* txt, char*& texture_pixels, int& texture_pitch, char* orig_pixels,
    int width, int height, int img_width,
    const sky::vector& scaled_x_axis, const sky::vector& scaled_y_axis, const sky::vector& axes_center,
    int half_scr_width, int half_scr_height,
    sky::screen& scr, SDL_Rect& srcrect, SDL_Rect& dstrect,
    float img_scale_x, float img_scale_y);