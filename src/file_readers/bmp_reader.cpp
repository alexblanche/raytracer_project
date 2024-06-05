#include "file_readers/bmp_reader.hpp"
#include "screen/color.hpp"

#include <stdio.h>
#include <string.h>
#include <cstdint>

#include <stdexcept>

/* Writes in the variables the width and height of the .bmp image contained in file_name */
std::optional<dimensions> read_bmp_size(const char* file_name) {
    FILE* file = fopen(file_name, "rb");

    if (file == NULL) {
        printf("Error, file %s not found\n", file_name);
        return std::nullopt;
    }

    try {
    
        int ret;

        /* 18 bytes ignored:
        Type (2), Size (4), Reserved 1 (2), Reserved 2 (2), Offset (4), Size (4)
        */
        char buffer18[18];
        ret = fread((void*) buffer18, 18, 1, file);
        if (ret != 1) {
            throw std::runtime_error("Reading error in read_bmp_size (18 first bytes)");
        }

        /* Width: 4 bytes */
        unsigned int bmpwidth;
        ret = fread((void*) &bmpwidth, sizeof(int), 1, file);
        if (ret != 1) {
            throw std::runtime_error("Reading error in read_bmp_size (width)");
        }
        const int width = (int) bmpwidth;

        /* Height: 4 bytes */
        unsigned int bmpheight;
        ret = fread((void*) &bmpheight, sizeof(int), 1, file);
        if (ret != 1) {
            throw std::runtime_error("Reading error in read_bmp_size (height)");
        }
        const int height = (int) bmpheight;

        fclose(file);
        return dimensions(width, height);
    }
    catch(const std::exception& e) {
        printf("%s\n", e.what());
        fclose(file);
        return std::nullopt;
    }
    
}

/* Extracts the data from the given .bmp file into the matrix data, which must have the right size */
bool read_bmp(const char* file_name, std::vector<std::vector<rt::color>>& data) {
    FILE* file = fopen(file_name, "rb");

    if (file == NULL) {
        printf("Error, file %s not found\n", file_name);
        return false;
    }

    try {

        int ret;

        /* 18 bytes ignored:
        Type (2), Size (4), Reserved 1 (2), Reserved 2 (2), Offset (4), Size (4)
        */
        char buffer18[18];
        ret = fread((void*) buffer18, 18, 1, file);
        if (ret != 1) {
            throw std::runtime_error("Reading error in read_bmp (18 first bytes)");
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
            throw std::runtime_error("Reading error in read_bmp (width)");
        }

        /* Height: 4 bytes */
        unsigned int bmpheight;
        ret = fread((void*) &bmpheight, sizeof(int), 1, file);
        if (ret != 1) {
            throw std::runtime_error("Reading error in read_bmp (height)");
        }

        /* 28 bytes ignored:
        Number of color planes (2), Number of bits per pixel (2), Compression method used (4),
        Size of the image (4), Horizontal resolution (4), Vertical resolution (4),
        Number of colors used (4), Number of important colors used (4)
        */
        char buffer28[28];
        ret = fread((void*) buffer28, 28, 1, file);
        if (ret != 1) {
            throw std::runtime_error("Reading error in read_bmp_size (28 bytes after height)");
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
        for (size_t j = 0; j < bmpheight; j++) {
            for (size_t i = 0; i < bmpwidth; i++) {
                const unsigned char b = fgetc(file);
                const unsigned char g = fgetc(file);
                const unsigned char r = fgetc(file);
                data[i][bmpheight - j - 1] = rt::color(r, g, b);
            }
            /* Skipping p bytes of padding */
            if (padding) {
                char buffer[p]; // p is constant
                ret = fread((void*) buffer, p, 1, file);
                if (ret != 1) {
                    throw std::runtime_error("Reading error in read_bmp (padding bytes)");
                }
            }
        }

        fclose(file);
        return true;

    }
    catch(const std::exception& e) {
        printf("%s\n", e.what());
        fclose(file);
        return false;
    }
}

#define HANDLE_ERROR if (ret < 0) { throw std::runtime_error(""); }

/* Writes the data into a .bmp file with the given name
   The value (double) of each component of each color of data is divided by number_of_rays before being written in the file
   Returns true if the operation was successful */
bool write_bmp(const char* file_name, std::vector<std::vector<rt::color>>& data, const unsigned int number_of_rays) {

    const double n = (double) number_of_rays;
    const unsigned int width = data.size();
    const unsigned int height = data[0].size();
    bool padding = false;
    size_t vp = (3*width) % 4;
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

    try {

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
            (file_size << 8) % 256,
            (file_size << 16) % 256,
            (file_size << 24) % 256
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
            (width << 8) % 256,
            (width << 16) % 256,
            (width << 24) % 256
        );
        HANDLE_ERROR

        /* 4 bytes: Height */
        ret = fprintf(file, "%c%c%c%c",
            height % 256,
            (height << 8) % 256,
            (height << 16) % 256,
            (height << 24) % 256
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
            (data_size << 8) % 256,
            (data_size << 16) % 256,
            (data_size << 24) % 256
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
        const size_t p = padding ? vp : 1;
        char padding_zeroes[p];
        for (size_t k = 0; k < p; k++) {
            padding_zeroes[k] = 0;
        }

        for (size_t j = 0; j < height; j++) {
            for (size_t i = 0; i < width; i++) {
                const rt::color& c = data[i][height - j - 1];
                const char r = (char) std::min(c.get_red()   / n, 255.0);
                const char g = (char) std::min(c.get_green() / n, 255.0);
                const char b = (char) std::min(c.get_blue()  / n, 255.0);
                ret = fprintf(file, "%c%c%c", b, g, r);
                HANDLE_ERROR
            }
            /* Writing p bytes '0' of padding */
            if (padding) {
                ret = fwrite((void*) padding_zeroes, p, 1, file);
                if (ret != 1) {
                    throw std::runtime_error("");
                }
            }
        }

        fclose(file);
        return true;

    }
    catch(const std::exception& e) {
        fclose(file);
        printf("Writing error in file %s\n", file_name);
        return false;
    }
}
