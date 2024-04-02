#include "postprocess/glow.hpp"

/* Glow rendering for bright lights */

/* Returns the normalized color (without saturation to white) */
rt::color get_normalized_color(const rt::color& col, const unsigned int number_of_rays) {
    const double max = std::max(col.get_red(), std::max(col.get_green(), col.get_blue()));
    return (col * (255/max));
}

std::vector<std::vector<rt::color>> extract_bright_pixels(const std::vector<std::vector<rt::color>>& image,
    const unsigned int number_of_rays) {

    const unsigned int width = image.size();
    const unsigned int height = image.at(0).size();
    std::vector<std::vector<rt::color>> bright_pixels(width, std::vector<rt::color>(height));
    
    for(unsigned int i = 0; i < width; i++) {
        for (unsigned int j = 0; j < height; j++) {
            const rt::color col = image.at(i).at(j);
            const double max = std::max(col.get_red(), std::max(col.get_green(), col.get_blue()));
            if (max > 255) {
                bright_pixels.at(i).at(j) = col;
            }
        }
    }

    return bright_pixels;
}


/* Applies a glowing effect on bright lights on the image
   - The image contains the sum of the number_of_rays colors computed at each pixel
   - When divided by number_of_rays, the "bright" pixels are the ones which exceed 255 in one of the RGB components
   - A blur is applied to the bright pixels, depending on the brightness, and added to the original image
 */
std::vector<std::vector<rt::color>> apply_glow(const std::vector<std::vector<rt::color>>& image,
    const unsigned int number_of_rays) {

    return {};
}
