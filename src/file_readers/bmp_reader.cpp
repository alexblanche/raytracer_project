#include "file_readers/bmp_reader.hpp"
#include "screen/color.hpp"

#include <stdio.h>
#include <string.h>
#include <cstdint>

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

/* Extracts the data from the given .bmp file into the matrix data, which must have the right size */
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

    /* Printing data */
    /*
    for (unsigned int i = 0; i < 18; i++) {
        printf("%d ", buffer18[i] & 0xff);
    }
    printf("\n");
    for (unsigned int i = 0; i < 18; i++) {
        printf("%c  ", buffer18[i] & 0xff);
    }
    printf("\n");
    */
    /******************/

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

    /* Printing data */
    /*
    printf("\n");
    for (unsigned int i = 0; i < 28; i++) {
        printf("%d ", buffer28[i] & 0xff);
    }
    printf("\n");
    for (unsigned int i = 0; i < 28; i++) {
        printf("%c  ", buffer28[i] & 0xff);
    }
    printf("\n");
    */
    /******************/

    /* Padding at the end of each row in the file */
    bool padding = false;
    unsigned int vp = (3*bmpwidth) % 4;
    if (vp != 0) {
        padding = true;
        vp = 4 - vp;
    }
    /* I need p constant because it is the size of an array defined below */
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
            char buffer[p]; // p is constant
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

#define HANDLE_ERROR if (ret < 0) {fclose(file); printf("Writing error in file %s\n", file_name); return false;}

/* Writes the data into a .bmp file with the given name
   The value (double) of each component of each color of data is divided by number_of_rays before being written in the file
   Returns true if the operation was successful */
bool write_bmp(const char* file_name, std::vector<std::vector<rt::color>>& data, const unsigned int number_of_rays) {

    const double n = (double) number_of_rays;
    const unsigned int width = data.size();
    const unsigned int height = data.at(0).size();
    bool padding = false;
    unsigned int vp = (3*width) % 4;
    if (vp != 0) {
        padding = true;
        vp = 4 - vp;
    }

    FILE* file = fopen(file_name, "wb");

    if (file == NULL) {
        printf("Error, file %s not found\n", file_name);
        return false;
    }

    /* All sizes are stored in little-endian */

    int ret;

    /** Header **/

    /* 2 bytes: BM */
    ret = fprintf(file, "BM");
    HANDLE_ERROR

    /* 4 bytes: File size */
    /* Size = 14 (header) + 40 (infoheader) + 3 * width * height (pixel data) + p * height */
    const unsigned int file_size = 14 + 40 + 3 * width * height + vp * height;
    ret = fprintf(file, "%c%c%c%c",
        file_size % 256,
        (file_size / 256) % 256,
        (file_size / (256 * 256)) % 256,
        (file_size / (256 * 256 * 256)) % 256
    );
    HANDLE_ERROR

    /* 4 bytes: 0 0 0 0 */
    ret = fprintf(file, "%c%c%c%c", 0, 0, 0, 0);
    HANDLE_ERROR

    /* 4 bytes: Offset from beginning of file to the beginning of the bitmap data = 54 */
    ret = fprintf(file, "%c%c%c%c", 54, 0, 0, 0);
    HANDLE_ERROR

    /** InfoHeader **/

    /* 4 bytes: Size of InfoHeader = 40 */
    ret = fprintf(file, "%c%c%c%c", 40, 0, 0, 0);
    HANDLE_ERROR

    /* 4 bytes: Width */
    ret = fprintf(file, "%c%c%c%c",
        width % 256,
        (width / 256) % 256,
        (width / (256 * 256)) % 256,
        (width / (256 * 256 * 256)) % 256
    );
    HANDLE_ERROR

    /* 4 bytes: Height */
    ret = fprintf(file, "%c%c%c%c",
        height % 256,
        (height / 256) % 256,
        (height / (256 * 256)) % 256,
        (height / (256 * 256 * 256)) % 256
    );
    HANDLE_ERROR

    /* 2 bytes: Planes = 1 */
    ret = fprintf(file, "%c%c", 1, 0);
    HANDLE_ERROR

    /* 2 bytes: Bits per Pixel (24 for 24 bits RGB) */
    ret = fprintf(file, "%c%c", 24, 0);
    HANDLE_ERROR

    /* 4 bytes: compression (0 for no compression) */
    ret = fprintf(file, "%c%c%c%c", 0, 0, 0, 0);
    HANDLE_ERROR

    /* 4 bytes: Size of the pixel data */
    /* Size = 3 * width * height (pixel data) + p * height */
    const unsigned int data_size = 3 * width * height + vp * height;
    ret = fprintf(file, "%c%c%c%c",
        data_size % 256,
        (data_size / 256) % 256,
        (data_size / (256 * 256)) % 256,
        (data_size / (256 * 256 * 256)) % 256
    );
    HANDLE_ERROR

    /* 4 bytes: Horizontal resolution (in pixels/meter)
       Unimportant: leaving it as 0 */
    ret = fprintf(file, "%c%c%c%c", 0, 0, 0, 0);
    HANDLE_ERROR

    /* 4 bytes: Vertical resolution (in pixels/meter)
       Unimportant: leaving it as 0 */
    ret = fprintf(file, "%c%c%c%c", 0, 0, 0, 0);
    HANDLE_ERROR

    /* 4 bytes: Number of actually used colors
       Unimportant: leaving it as 0 */
    ret = fprintf(file, "%c%c%c%c", 0, 0, 0, 0);
    HANDLE_ERROR

    /* 4 bytes: Important colors
       Unimportant: leaving it as 0 */
    ret = fprintf(file, "%c%c%c%c", 0, 0, 0, 0);
    HANDLE_ERROR



    /** Color data **/
    /* Each pixel is represented as 3 bytes BGR, each line (sequence of 3*width bytes) is followed by p bytes '0' of padding */

    /* Padding bytes at the end of each line (if padding = true) */
    const unsigned int p = padding ? vp : 1;
    char padding_zeroes[p];
    for (unsigned int k = 0; k < p; k++) {
        padding_zeroes[k] = 0;
    }

    for (unsigned int j = 0; j < height; j++) {
        for (unsigned int i = 0; i < width; i++) {
            rt::color c = data.at(i).at(height - j - 1);
            const char r = (char) std::min(c.get_red() / n, 255.0);
            const char g = (char) std::min(c.get_green() / n, 255.0);
            const char b = (char) std::min(c.get_blue() / n, 255.0);
            ret = fprintf(file, "%c%c%c", b, g, r);
            HANDLE_ERROR
        }
        /* Writing p bytes '0' of padding */
        if (padding) {
            ret = fwrite((void*) padding_zeroes, p, 1, file);
            if (ret < (int) p) {fclose(file); printf("Writing error in file %s\n", file_name); return false;}
        }
    }

    fclose(file);
    return true;
}
