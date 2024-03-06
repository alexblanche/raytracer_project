#include "file_readers/bmp_reader.hpp"
#include "screen/color.hpp"

#include <stdio.h>
#include <string.h>

/* Writes in the variables the width and height of the .bmp image contained in file_name */
bool read_bmp_size(const char* file_name, int& width, int& height) {
    FILE* file = fopen(file_name, "rb");

    if (file == NULL) {
        printf("Error, file %s not found\n", file_name);
        return false;
    }
    
    int ret;

    /* 18 bytes ignored:
       Type (2), Size (4), Reserved 1 (2), Reserved 2 (2), Offset (4), Size (4)
    */
    char buffer18[18];
    ret = fread((void*) buffer18, 18, 1, file);
    if (ret != 1) {
        printf("Reading error in read_bmp_size (18 first bytes)\n");
        fclose(file);
        return false;
    }

    /* Width: 4 bytes */
    unsigned int bmpwidth;
    ret = fread((void*) &bmpwidth, sizeof(int), 1, file);
    if (ret != 1) {
        printf("Reading error in read_bmp_size (width)\n");
        fclose(file);
        return false;
    }
    width = (int) bmpwidth;

    /* Height: 4 bytes */
    unsigned int bmpheight;
    ret = fread((void*) &bmpheight, sizeof(int), 1, file);
    if (ret != 1) {
        printf("Reading error in read_bmp_size (height)\n");
        fclose(file);
        return false;
    }
    height = (int) bmpheight;

    fclose(file);
    return true;
}

/* Extracts the data from the given .bmp file: stores the width and height in the provided
   references, and returns a matrix of width rows and height columns containing colors  */
bool read_bmp(const char* file_name, std::vector<std::vector<rt::color>>& data) {
    FILE* file = fopen(file_name, "rb");

    if (file == NULL) {
        printf("Error, file %s not found\n", file_name);
        return false;
    }

    int ret;

    /* 18 bytes ignored:
       Type (2), Size (4), Reserved 1 (2), Reserved 2 (2), Offset (4), Size (4)
    */
    char buffer18[18];
    ret = fread((void*) buffer18, 18, 1, file);
    if (ret != 1) {
        printf("Reading error in read_bmp (18 first bytes)\n");
        fclose(file);
        return false;
    }

    /* Width: 4 bytes */
    unsigned int bmpwidth;
    ret = fread((void*) &bmpwidth, sizeof(int), 1, file);
    if (ret != 1) {
        printf("Reading error in read_bmp (width)\n");
        fclose(file);
        return false;
    }

    /* Height: 4 bytes */
    unsigned int bmpheight;
    ret = fread((void*) &bmpheight, sizeof(int), 1, file);
    if (ret != 1) {
        printf("Reading error in read_bmp (height)\n");
        fclose(file);
        return false;
    }

    /* 28 bytes ignored:
       Number of color planes (2), Number of bits per pixel (2), Compression method used (4),
       Size of the image (4), Horizontal resolution (4), Vertical resolution (4),
       Number of colors used (4), Number of important colors used (4)
    */
    char buffer28[28];
    ret = fread((void*) buffer28, 28, 1, file);
    if (ret != 1) {
        printf("Reading error in read_bmp_size (28 bytes after height)\n");
        fclose(file);
        return false;
    }

    /* Padding at the end of each row in the file */
    bool padding = false;
    unsigned int vp = (3*bmpwidth) % 4;
    if (vp != 0) {
        padding = true;
        vp = 4 - vp;
    }
    const unsigned int p = vp;

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
            ret = fread((void*) buffer, p, 1, file);
            if (ret != 1) {
                printf("Reading error in read_bmp (padding bytes at line %u)\n", j);
                fclose(file);
                return false;
            }
        }
    }

    fclose(file);
    return true;
}

/* Writes the data into a .bmp file with the given name */
/* TODO
void write_bmp(const char* file_name, std::vector<std::vector<rt::color>>& data,
    const int width, const int height) {

}
*/