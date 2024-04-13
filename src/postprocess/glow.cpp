#include "postprocess/glow.hpp"

#include "file_readers/raw_data.hpp"
#include "file_readers/bmp_reader.hpp"

#include <cmath>
#include <stdio.h>
#include <cstdlib>

#include <stdio.h>
#include <string.h>

#define TWOPI 6.2831853071795862

/* Glow rendering for bright lights */

/* Struct containing a color and an alpha channel for transparency */
struct pixel {
    rt::color col;
    double alpha;
    double total_alpha;

    pixel() : col(rt::color::BLACK), alpha(0), total_alpha(0) {}

    pixel(const rt::color& col, const double& alpha)
        : col(col), alpha(alpha), total_alpha(alpha) {}

    void add(const rt::color& ncol, const double& nalpha) {
        if (col == rt::color::BLACK) {
            col = ncol;
            alpha = nalpha;
            total_alpha = nalpha;
        }
        else {
            col = ((col * total_alpha) + (ncol * nalpha)) / (total_alpha + nalpha);
            alpha = std::max(alpha, nalpha);
            total_alpha = total_alpha + nalpha;
        }
    }

    rt::color superimpose(const rt::color& orig_col, const unsigned int number_of_rays, const double& threshold) {
        if (alpha == 0
            || std::max(orig_col.get_red(), std::max(orig_col.get_green(), orig_col.get_blue())) > 255 * number_of_rays * threshold) {
            
            return (orig_col / number_of_rays).max_out();
        }
        else {
            return ((orig_col / number_of_rays) * (1 - alpha)) + (col * alpha);
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
    const unsigned int number_of_rays, const double& threshold) {

    /* The glow effect is disabled when light intensity is below 4 */
    const double thresh = 255 * number_of_rays * std::max(4.0, threshold);

    const unsigned int width = image.size();
    const unsigned int height = image.at(0).size();
    std::vector<std::vector<rt::color>> bright_pixels(width, std::vector<rt::color>(height));
    
    for(unsigned int i = 0; i < width; i++) {
        for (unsigned int j = 0; j < height; j++) {
            const rt::color col = image.at(i).at(j);
            const double max = std::max(col.get_red(), std::max(col.get_green(), col.get_blue()));
            if (max > thresh) {
                bright_pixels.at(i).at(j) = col;
            }
        }
    }

    return bright_pixels;
}

/* Applies blurred normalized_color to output_image at center pixel (a,b), with given intensity */
void blur_pixel(std::vector<std::vector<pixel>>& glow_image,
    const std::vector<std::vector<rt::color>>& bright_pixels,
    const int a, const int b,
    const rt::color& normalized_color, const double& intensity,
    const double& glow_intensity) {

    const int width = glow_image.size();
    const int height = glow_image.at(0).size();

    const double std_dev_sq = glow_intensity * intensity * intensity;

    const double div = - 1 / (2 * std_dev_sq);
    const double radius = sqrt(4 * std_dev_sq * log(10)); // sqrt(2 * std_dev_sq * -log(0.01))
    
    for(int i = std::max((int) -radius, -a); i < std::min((int) radius, width - a); i++) {
        for(int j = std::max((int) -radius, -b); j < std::min((int) radius, height - b); j++) {

            if (bright_pixels.at(i + a).at(j + b) == rt::color::BLACK) {

                const double gaussian = exp(div * (i * i + j * j));
                glow_image.at(i + a).at(j + b).add(normalized_color, gaussian);
            }
        }
    }
}

/* Generates and returns the glow image */
std::vector<std::vector<pixel>> generate_glow(const std::vector<std::vector<rt::color>>& bright_pixels,
    const unsigned int number_of_rays, const double& glow_intensity) {
    
    const unsigned int width = bright_pixels.size();
    const unsigned int height = bright_pixels.at(0).size();
    std::vector<std::vector<pixel>> glow(width, std::vector<pixel>(height));

    printf("0 / %u", width);
    fflush(stdout);

    for (unsigned int i = 0; i < width; i++) {
        for (unsigned int j = 0; j < height; j++) {
            if (not (bright_pixels.at(i).at(j) == rt::color::BLACK)) {
                double intensity;
                const rt::color normalized_color = get_normalized_color(bright_pixels.at(i).at(j), number_of_rays, intensity);
                blur_pixel(glow, bright_pixels, i, j, normalized_color, intensity, glow_intensity);
            }
        }
        printf("\r%u / %u", i+1, width);
        fflush(stdout);
    }
    printf("\n");

    return glow;
}


/* Applies a glowing effect on bright lights on the image
   - The image contains the sum of the number_of_rays colors computed at each pixel
   - When divided by number_of_rays, the "bright" pixels are the ones which exceed 255 in one of the RGB components
   - A blur is applied to the bright pixels, depending on the brightness, and added to the original image
 */
std::vector<std::vector<rt::color>> apply_glow(const std::vector<std::vector<rt::color>>& image,
    const unsigned int number_of_rays, const double& threshold, const double& glow_intensity) {

    std::vector<std::vector<rt::color>> bright_pixels = extract_bright_pixels(image, number_of_rays, threshold);
    std::vector<std::vector<pixel>> glow_image = generate_glow(bright_pixels, number_of_rays, glow_intensity);

    const unsigned int width = image.size();
    const unsigned int height = image.at(0).size();
    std::vector<std::vector<rt::color>> output_image(width, std::vector<rt::color>(height));

    for (unsigned int i = 0; i < width; i++) {
        for (unsigned int j = 0; j < height; j++) {
            output_image.at(i).at(j) = glow_image.at(i).at(j).superimpose(image.at(i).at(j), number_of_rays, threshold);
        }
    }

    return output_image;
}

/* Main function, reads the raw data file, applies glow to it and saves it as bmp file,
   unless optional arguments threshold and glow_intensity are provided.
   Syntax:
   ./postprocess [-threshold 10] [-glow 3] source.rtdata dest.bmp
   threshold removes glow effect for light intensity inferior to it
   glow adjusts the intensity of the glow effect (standard deviation of the gaussian applied to each bright pixel)
*/

int main(int argc, char* argv[]) {

    const unsigned int input_arg = argc - 2;
    const unsigned int output_arg = argc - 1;

    double threshold = 4;
    double glow_intensity = 1;

    if (argc <= 2) {
        printf("No destination file name provided\n");
        return EXIT_SUCCESS;
    }
    else if (argc > 3) {
        for (unsigned int i = 1; i <= 3; i += 2) {
            if (strcmp(argv[i], "-threshold") == 0) {
                threshold = atof(argv[i+1]);
            }
            else if (strcmp(argv[i], "-glow") == 0) {
                glow_intensity = atof(argv[i+1]);
            }
        }
    }

    printf("Reading file...");
    fflush(stdout);

    unsigned int number_of_rays;
    bool success;
    std::vector<std::vector<rt::color>>& image = read_raw(argv[input_arg], number_of_rays, success);
    
    if (not success) {
        printf("Error reading file %s\n", argv[input_arg]);
        return EXIT_SUCCESS;
    }

    printf("\rThreshold: %lf, glow intensity: %lf\nApplying glow...\n", threshold, glow_intensity);
    fflush(stdout);

    std::vector<std::vector<rt::color>> postprocessed_image = apply_glow(image, number_of_rays, threshold, glow_intensity);

    printf("Done.\n");

    const bool success_export = write_bmp(argv[output_arg], postprocessed_image, 1);

    if (success_export) {
        printf("File %s created\n", argv[output_arg]);
    }
    else {
        printf("Error, export failed\n");
    }

    return EXIT_SUCCESS;
}