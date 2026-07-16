#include "other/obsolete/postprocess/glow.hpp"

#include "file_readers/image_files/raw_data.hpp"
#include "file_readers/image_files/bmp_reader.hpp"

#include <cmath>

/* Glow rendering for bright lights */

/* Struct containing a color and an alpha channel for transparency */
struct pixel {
    rt::color col;
    double alpha;
    double total_alpha;

    pixel() : col(rt::BLACK), alpha(0), total_alpha(0) {}

    pixel(const rt::color& col, const double& alpha)
        : col(col), alpha(alpha), total_alpha(alpha) {}

    void add(const rt::color& ncol, const double nalpha) {
        if (col == rt::BLACK) {
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

    inline rt::color superimpose(const rt::color& orig_col, const unsigned int number_of_rays, const double threshold) {
        const auto [ r, g, b ] = orig_col;
        return (alpha == 0.0
            || std::max(r, std::max(g, b)) > 255.0_r * number_of_rays * threshold) ?
            
               (orig_col / number_of_rays).get_capped()
            : ((orig_col / number_of_rays) * (1.0 - alpha)) + (col * alpha);
    }
};

/* Struct containing the normalized color and the intensity */
struct normalized {
    rt::color normalized_color;
    double intensity;

    normalized(const rt::color& col, const double intensity)
        : normalized_color(col), intensity(intensity) {}
};

/* Returns the normalized color (without saturation to white) and its intensity */
normalized get_normalized_color(const rt::color& col, const unsigned int number_of_rays) {

    const auto [ r, g, b ] = col;
    const double max = std::max(r, std::max(g, b));
    const double intensity = max / (255 * number_of_rays);
    return normalized(col * (255.0 / max), intensity);
}

/* Extracts the saturated pixels of the image */
matrix extract_bright_pixels(const image& image, const double threshold) {

    /* The glow effect is disabled when light intensity is below 4 */
    const double thresh = 255.0 * image.number_of_samples * std::max(4.0, threshold);

    const int width  = image.width();
    const int height = image.height();
    matrix bright_pixels(width, height);

    for (int j = 0; const matrix::const_row row : image.data) {
        for (int i = 0; const rt::color& col : row) {
            const auto [ r, g, b ] = col;
            const double max = std::max(r, std::max(g, b));
            if (max > thresh)
                bright_pixels[j, i] = col;
            i++;
        }
        j++;
    }

    return bright_pixels;
}

/* Applies blurred normalized_color to output_image at center pixel (a,b), with given intensity */
void blur_pixel(std::vector<std::vector<pixel>>& glow_image,
    const matrix& bright_pixels,
    const int a, const int b,
    const rt::color& normalized_color, const double intensity,
    const double glow_intensity) {

    const int width = glow_image.size();
    const int height = glow_image[0].size();

    const double std_dev_sq = glow_intensity * intensity * intensity;

    const double div = - 1 / (2 * std_dev_sq);
    const double radius = sqrt(4 * std_dev_sq * log(10)); // sqrt(2 * std_dev_sq * -log(0.01))
    
    const int imin = std::max(static_cast<int>(-radius), -a);
    const int imax = std::min(static_cast<int>( radius), width - a);
    const int jmin = std::max(static_cast<int>(-radius), -b);
    const int jmax = std::min(static_cast<int>( radius), height - b);

    for(int j = jmin; j < jmax; j++) {
        for(int i = imin; i < imax; i++) {

            if (bright_pixels[j + b, i + a] == rt::BLACK) {

                const double gaussian = exp(div * (i * i + j * j));
                glow_image[j + b][i + a].add(normalized_color, gaussian);
            }
        }
    }
}

/* Generates and returns the glow image */
std::vector<std::vector<pixel>> generate_glow(const matrix& bright_pixels,
    const unsigned int number_of_rays, const double glow_intensity) {
    
    const int width  = static_cast<int>(bright_pixels.width);
    const int height = static_cast<int>(bright_pixels.height);
    std::vector<std::vector<pixel>> glow(height, std::vector<pixel>(width));

    printf("0 / %d", width);
    fflush(stdout);

    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
        
            if (not (bright_pixels[j, i] == rt::BLACK)) {
                const normalized nc = get_normalized_color(bright_pixels[j, i], number_of_rays);
                blur_pixel(glow, bright_pixels, i, j, nc.normalized_color, nc.intensity, glow_intensity);
            }
        }
        printf("\r%d / %d", j+1, height);
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
void apply_glow(image& image, double threshold, double glow_intensity) {

    matrix bright_pixels = extract_bright_pixels(image, threshold);
    std::vector<std::vector<pixel>> glow_image = generate_glow(bright_pixels, image.number_of_samples, glow_intensity);

    const int width  = image.width();
    const int height = image.height();

    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++)
            image[j, i] = glow_image[j][i].superimpose(image[j, i], image.number_of_samples, threshold);
    }
}

/* Main function, reads the raw data file, applies glow to it and saves it as bmp file,
   unless optional arguments threshold and glow_intensity are provided.
   Syntax:
   ./postprocess [-threshold 10] [-glow 3] source.rtdata dest.bmp
   threshold removes glow effect for light intensity inferior to it
   glow adjusts the intensity of the glow effect (standard deviation of the gaussian applied to each bright pixel)
*/

int main(int argc, char* argv[]) {

    const unsigned int input_arg  = argc - 2;
    const unsigned int output_arg = argc - 1;

    double threshold = 4;
    double glow_intensity = 1;

    if (argc <= 2) {
        printf("No destination file name provided\n");
        return EXIT_SUCCESS;
    }
    else if (argc > 3) {
        for (int i = 1; i <= 3; i += 2) {
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
    std::expected<image, file_reader::error> image_opt = raw_data::read_file(argv[input_arg]);

    if (not image_opt.has_value()) {
        printf("Error reading file %s\n", argv[input_arg]);
        return EXIT_FAILURE;
    }

    image& image = image_opt.value();

    printf("\rThreshold: %lf, glow intensity: %lf\nApplying glow...\n", threshold, glow_intensity);
    fflush(stdout);

    apply_glow(image, threshold, glow_intensity);
    image.number_of_samples = 1;

    printf("Done.\n");

    const exit_status success_export = bmp::export_data(argv[output_arg], image);

    if (success_export == exit_status::Success) {
        printf("File %s created\n", argv[output_arg]);
    }
    else {
        printf("Error, export failed\n");
    }

    return EXIT_SUCCESS;
}