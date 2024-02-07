#include "../screen/headers/color.hpp"
#include "headers/blur.hpp"
#include <iostream>

/* Blurring algorithm */

/* Computes a box blur of the image, with box of side range (range should be an odd number) */
std::vector<std::vector<rt::color>> blur (const std::vector<std::vector<rt::color>>& image,
    const unsigned int range) {
        
    const int width = image.size();
    const int height = image.at(0).size();

    std::vector<std::vector<rt::color>> blurred_image(width, std::vector<rt::color>(height));
    const int halfrange = range / 2;
    const unsigned int rangesq = range * range;

    /* Main loop */

    for (int i = halfrange; i < width - halfrange; i++) {
        for (int j = halfrange; j < height - halfrange; j++) {

            unsigned int r = 0;
            unsigned int g = 0;
            unsigned int b = 0;

            for (int x = -halfrange; x < halfrange+1; x++) {
                for (int y = -halfrange; y < halfrange+1; y++) {
                    const rt::color col = image.at(i+x).at(j+y);
                    r += col.get_red();
                    g += col.get_green();
                    b += col.get_blue();
                }
            }

            blurred_image.at(i).at(j) = rt::color(r / rangesq, g / rangesq, b / rangesq);
        }
    }

    /* Corrections for the sides of the image */

    for (int i = 0; i < halfrange; i++) {
        for (int j = halfrange; j < height - halfrange; j++) {

            unsigned int r = 0;
            unsigned int g = 0;
            unsigned int b = 0;
            unsigned int cpt = 0;

            for (int x = -halfrange; x < halfrange+1; x++) {
                if (i + x >= 0) {
                    for (int y = -halfrange; y < halfrange+1; y++) {
                        const rt::color col = image.at(i+x).at(j+y);
                        r += col.get_red();
                        g += col.get_green();
                        b += col.get_blue();
                        cpt ++;
                    }
                }
            }

            blurred_image.at(i).at(j) = rt::color(r / cpt, g / cpt, b / cpt);
        }
    }

    for (int i = width - halfrange; i < width; i++) {
        for (int j = halfrange; j < height - halfrange; j++) {

            unsigned int r = 0;
            unsigned int g = 0;
            unsigned int b = 0;
            unsigned int cpt = 0;

            for (int x = -halfrange; x < halfrange+1; x++) {
                if (i + x < width) {
                    for (int y = -halfrange; y < halfrange+1; y++) {
                        const rt::color col = image.at(i+x).at(j+y);
                        r += col.get_red();
                        g += col.get_green();
                        b += col.get_blue();
                        cpt ++;
                    }
                }
            }

            blurred_image.at(i).at(j) = rt::color(r / cpt, g / cpt, b / cpt);
        }
    }

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < halfrange; j++) {

            unsigned int r = 0;
            unsigned int g = 0;
            unsigned int b = 0;
            unsigned int cpt = 0;

            for (int y = -halfrange; y < halfrange+1; y++) {
                if (j + y >= 0) {
                    for (int x = -halfrange; x < halfrange+1; x++) {
                        if (i + x >= 0 && i + x < width) {
                            const rt::color col = image.at(i+x).at(j+y);
                            r += col.get_red();
                            g += col.get_green();
                            b += col.get_blue();
                            cpt ++;
                        }
                    }
                }
            }

            blurred_image.at(i).at(j) = rt::color(r / cpt, g / cpt, b / cpt);
        }
    }

    for (int i = 0; i < width; i++) {
        for (int j = height - halfrange; j < height; j++) {

            unsigned int r = 0;
            unsigned int g = 0;
            unsigned int b = 0;
            unsigned int cpt = 0;

            for (int y = -halfrange; y < halfrange+1; y++) {
                if (j + y < height) {
                    for (int x = -halfrange; x < halfrange+1; x++) {
                        if (i + x >= 0 && i + x < width) {
                            const rt::color col = image.at(i+x).at(j+y);
                            r += col.get_red();
                            g += col.get_green();
                            b += col.get_blue();
                            cpt ++;
                        }
                    }
                }
            }

            blurred_image.at(i).at(j) = rt::color(r / cpt, g / cpt, b / cpt);
        }
    }

    return blurred_image;
}
