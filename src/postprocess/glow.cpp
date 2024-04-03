#include "postprocess/glow.hpp"

#include <cmath>
#include <stdio.h>

#define TWOPI 6.2831853071795862

/* Glow rendering for bright lights */

/* Struct containing a color and an alpha channel for transparency */
struct pixel {
    rt::color col;
    double alpha;

    pixel() : col(rt::color(0,0,0)), alpha(0) {}

    pixel(const rt::color& col, const double& alpha)
        : col(col), alpha(alpha) {}

    void add(const rt::color& ncol, const double& nalpha) {
        col = (col * alpha + ncol * nalpha) / (alpha + nalpha);
        alpha = std::max(alpha, nalpha);
    }

    rt::color superimpose(const rt::color& orig_col, const unsigned int number_of_rays) {
        if (alpha == 0
            || std::max(orig_col.get_red(), std::max(orig_col.get_green(), orig_col.get_blue())) > 255 * number_of_rays) {
            
            return (orig_col / number_of_rays).max_out();
        }
        else {
            return (orig_col / number_of_rays) * (1 - alpha) + col * alpha;
        }
    }
};


/* Returns the normalized color (without saturation to white), and writes the intensity in the given variable */
rt::color get_normalized_color(const rt::color& col, const unsigned int number_of_rays,
    double& intensity) {

    const double max = std::max(col.get_red(), std::max(col.get_green(), col.get_blue()));
    intensity = max / (255 * number_of_rays);
    return (col * (255/max));
}

/* Extracts the saturated pixels of the image */
std::vector<std::vector<rt::color>> extract_bright_pixels(const std::vector<std::vector<rt::color>>& image,
    const unsigned int number_of_rays) {

    const unsigned int width = image.size();
    const unsigned int height = image.at(0).size();
    std::vector<std::vector<rt::color>> bright_pixels(width, std::vector<rt::color>(height));
    
    for(unsigned int i = 0; i < width; i++) {
        for (unsigned int j = 0; j < height; j++) {
            const rt::color col = image.at(i).at(j);
            const double max = std::max(col.get_red(), std::max(col.get_green(), col.get_blue()));
            if (max > 255 * number_of_rays) {
                bright_pixels.at(i).at(j) = col;
            }
        }
    }

    return bright_pixels;
}

/* Applies blurred normalized_color to output_image at center pixel (a,b), with given intensity */
void blur_pixel(std::vector<std::vector<pixel>>& output_image,
    const unsigned int a, const unsigned int b,
    const rt::color& normalized_color, const unsigned int intensity) {

    const unsigned int width = output_image.size();
    const unsigned int height = output_image.at(0).size();

    const double std_dev_sq = intensity * intensity;

    const double scal = 1 / (TWOPI * std_dev_sq);
    const double div = - 1 / (2 * std_dev_sq);
    
    const unsigned int radius = sqrt(2 * std_dev_sq * log(0.01 * TWOPI * std_dev_sq));
    for(unsigned int i = std::max(- radius, -a); i < std::min(radius, width - a); i++) {
        for(unsigned int j = std::max(- radius, -b); j < std::min(radius, height - b); j++) {
            const double gaussian = scal * exp(div * (i * i + j * j));
            output_image.at(i+a).at(j+b).add(normalized_color, gaussian);
        }
    }
}

/* Generates and returns the glow image */
std::vector<std::vector<pixel>> generate_glow(const std::vector<std::vector<rt::color>>& bright_pixels,
    const unsigned int number_of_rays) {
    
    const unsigned int width = bright_pixels.size();
    const unsigned int height = bright_pixels.at(0).size();
    std::vector<std::vector<pixel>> glow(width, std::vector<pixel>(height));

    for (unsigned int i = 0; i < width; i++) {
        for (unsigned int j = 0; j < height; j++) {
            if (not (bright_pixels.at(i).at(j) == rt::color::BLACK)) {
                double intensity;
                const rt::color normalized_color = get_normalized_color(bright_pixels.at(i).at(j), number_of_rays, intensity);
                blur_pixel(glow, i, j, normalized_color, intensity);
            }
        }
    }

    return glow;
}


/* Applies a glowing effect on bright lights on the image
   - The image contains the sum of the number_of_rays colors computed at each pixel
   - When divided by number_of_rays, the "bright" pixels are the ones which exceed 255 in one of the RGB components
   - A blur is applied to the bright pixels, depending on the brightness, and added to the original image
 */
std::vector<std::vector<rt::color>> apply_glow(const std::vector<std::vector<rt::color>>& image,
    const unsigned int number_of_rays) {

    std::vector<std::vector<rt::color>> bright_pixels = extract_bright_pixels(image, number_of_rays);
    std::vector<std::vector<pixel>> glow_image = generate_glow(bright_pixels, number_of_rays);

    const unsigned int width = image.size();
    const unsigned int height = image.at(0).size();
    std::vector<std::vector<rt::color>> output_image(width, std::vector<rt::color>(height));

    for (unsigned int i = 0; i < width; i++) {
        for (unsigned int j = 0; j < height; j++) {
            double intensity;
            output_image.at(i).at(j) = get_normalized_color(bright_pixels.at(i).at(j), number_of_rays, intensity);
            //output_image.at(i).at(j) = glow_image.at(i).at(j).col;
            //output_image.at(i).at(j) = glow_image.at(i).at(j).superimpose(rt::color(0,0,0), 1);
            //output_image.at(i).at(j) = glow_image.at(i).at(j).superimpose(image.at(i).at(j), number_of_rays);
        }
    }

    return output_image;
}
