#include "screen/screen.hpp"
#include "light/vector.hpp"

void render(SDL_Texture* txt, char*& texture_pixels, int& texture_pitch, char* orig_pixels,
    const int width, const int height, const int img_width,
    const rt::vector& scaled_x_axis, const rt::vector& scaled_y_axis, const rt::vector& axes_center,
    const int half_scr_width, const int half_scr_height,
    rt::screen& scr, SDL_Rect& srcrect, SDL_Rect& dstrect,
    const float img_scale_x, const float img_scale_y);