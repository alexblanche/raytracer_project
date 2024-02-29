#include "headers/bmp_reader.hpp"
#include "../screen/headers/color.hpp"

#include <stdio.h>
#include <string.h>

/* Extracts the data from the given .bmp file: stores the width and height in the provided
   references, and returns a matrix of width rows and height columns containing colors  */
std::vector<std::vector<rt::color>> read_bmp(const char* file_name, int& width, int& height) {
    FILE* file = fopen(file_name, "rb");

    /* 18 bytes ignored:
       Type (2), Size (4), Reserved 1 (2), Reserved 2 (2), Offset (4), Size (4)
    */
    char buffer18[18];
    fread((void*) buffer18, 18, 1, file);

    /* Width: 4 bytes */
    unsigned int bmpwidth;
    fread((void*) &bmpwidth, sizeof(int), 1, file);
    width = (int) bmpwidth;

    /* Height: 4 bytes */
    unsigned int bmpheight;
    fread((void*) &bmpheight, sizeof(int), 1, file);
    height = (int) bmpheight;

    /* 28 bytes ignored:
       Number of color planes (2), Number of bits per pixel (2), Compression method used (4),
       Size of the image (4), Horizontal resolution (4), Vertical resolution (4),
       Number of colors used (4), Number of important colors used (4)
    */
    char buffer28[28];
    fread((void*) buffer28, 28, 1, file);

    /* Padding at the end of each row in the file */
    unsigned int p = (3*bmpwidth) % 4;
    bool padding = false;
    if (p != 0) {
        padding = true;
        p = 4 - p;
    }

    /* Matrix in which we will store the data */
    std::vector<std::vector<rt::color>> data(bmpwidth, std::vector<rt::color>(bmpheight));

    /* Color data */
    for (unsigned int j = 0; j < bmpheight; j++) {
        for (unsigned int i = 0; i < bmpwidth; i++) {
            const unsigned char b = fgetc(file);
            const unsigned char g = fgetc(file);
            const unsigned char r = fgetc(file);
            data.at(i).at(bmpheight - j - 1) = rt::color(r, g, b);
        }
        /* Skipping p bytes of padding */
        if (padding) {
            char buffer[p];
            fread((void*) buffer, p, 1, file);
        }
    }

    fclose(file);
    return data;
}

/* Writes the data into a .bmp file with the given name */
/* TODO
void write_bmp(const char* file_name, std::vector<std::vector<rt::color>>& data,
    const int width, const int height) {

}
*/