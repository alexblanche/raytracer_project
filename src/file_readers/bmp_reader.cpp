#include "file_readers/bmp_reader.hpp"
#include "screen/color.hpp"

#include <stdio.h>
#include <string.h>
#include <cstdint>

#include <stdexcept>
#include <cmath>

#define MAX_PADDING 4

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
        Type (2), Size (4), Reserved 1 (2), Reserved 2 (2), Offset (4), Size of the header (4)
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
        Type (2), Size (4), Reserved 1 (2), Reserved 2 (2), Offset (4), Size of the header (4)
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
        Compressed size of the image (4), Horizontal resolution (4), Vertical resolution (4),
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
        //bool padding = false;
        unsigned int p = (3 * bmpwidth) % 4;
        if (p != 0) {
            //padding = true;
            p = 4 - p;
        }
        

        /* Color data */
        const unsigned int nb_bytes = ((3 * bmpwidth) + p) * bmpheight;
        std::vector<unsigned char> buffer(nb_bytes);
        ret = fread((void*) buffer.data(), 1, nb_bytes, file);
        if ((unsigned int) ret != nb_bytes) {
            throw std::runtime_error("Reading error in read_bmp (pixel data)");
        }

        unsigned int index = 0;
        for (unsigned int j = 0; j < bmpheight; j++) {
            const unsigned int indexj = bmpheight - j - 1;
            for (unsigned int i = 0; i < bmpwidth; i++) {
                const real b = buffer[index];
                index++;
                const real g = buffer[index];
                index++;
                const real r = buffer[index];
                index++;
                data[i][indexj] = rt::color(r, g, b);
            }
            index += p;
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

/* Returns the integer stored in the buffer at index start_index on nb_bytes bytes (in Little Endian convention) */
unsigned int value_of_bytes(const unsigned char buffer[], const int start_index, const int nb_bytes) {
    unsigned int value = 0;
    for (int i = start_index + nb_bytes - 1; i >= start_index; i--) {
        value = (value << 8) + buffer[i];
    }
    return value;
}

/* Prints the info contained in the header of the given .bmp file */
bool print_bmp_info(const char* file_name) {
    FILE* file = fopen(file_name, "rb");

    if (file == NULL) {
        printf("Error, file %s not found\n", file_name);
        return false;
    }

    try {

        int ret;

        unsigned char buffer[54];
        ret = fread((void*) buffer, 54, 1, file);
        if (ret != 1) {
            throw std::runtime_error("Reading error in read_bmp");
        }

        printf("Type:                  %c%c\n",     buffer[0], buffer[1]);
        printf("File size:             %u bytes\n", value_of_bytes(buffer, 2, 4));
        printf("Reserved 1:            0x%d%d\n",   buffer[6], buffer[7]);
        printf("Reserved 2:            0x%d%d\n",   buffer[8], buffer[9]);
        printf("Offset:                %u\n",       value_of_bytes(buffer, 10, 4));
        printf("Header size:           %u bytes\n", value_of_bytes(buffer, 14, 4));
        printf("Width:                 %u\n",       value_of_bytes(buffer, 18, 4));
        printf("Height:                %u\n",       value_of_bytes(buffer, 22, 4));
        printf("Color planes:          %u\n",       value_of_bytes(buffer, 26, 2));
        printf("Bits per pixel:        %u\n",       value_of_bytes(buffer, 28, 2));
        printf("Compression method:    %u\n",       value_of_bytes(buffer, 30, 4));
        printf("Compressed size:       %u bytes\n", value_of_bytes(buffer, 34, 4));
        printf("Horizontal resolution: %u\n",       value_of_bytes(buffer, 38, 4));
        printf("Vertical resolution:   %u\n",       value_of_bytes(buffer, 42, 4));
        printf("Colors used:           %u\n",       value_of_bytes(buffer, 46, 4));
        printf("Important colors:      %u\n",       value_of_bytes(buffer, 50, 4));

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
bool write_bmp(const char* file_name, std::vector<std::vector<rt::color>>& data,
    const unsigned int number_of_rays, const real gamma) {

    const double n = number_of_rays;
    const unsigned int width = data.size();
    const unsigned int height = data[0].size();
    bool padding = false;
    unsigned int p = (3 * width) % 4;
    if (p != 0) {
        padding = true;
        p = 4 - p;
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
        const unsigned int file_size = 14 + 40 + (3 * width + p) * height;
        ret = fprintf(file, "%c%c%c%c",
            file_size & 0xFF,
            (file_size >> 8) & 0xFF,
            (file_size >> 16) & 0xFF,
            (file_size >> 24) & 0xFF
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
            width & 0xFF,
            (width >> 8) & 0xFF,
            (width >> 16) & 0xFF,
            (width >> 24) & 0xFF
        );
        HANDLE_ERROR

        /* 4 bytes: Height */
        ret = fprintf(file, "%c%c%c%c",
            height & 0xFF,
            (height >> 8) & 0xFF,
            (height >> 16) & 0xFF,
            (height >> 24) & 0xFF
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
        const unsigned int data_size = (3 * width + p) * height;
        ret = fprintf(file, "%c%c%c%c",
            data_size & 0xFF,
            (data_size >> 8) & 0xFF,
            (data_size >> 16) & 0xFF,
            (data_size >> 24) & 0xFF
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
        char padding_zeroes[MAX_PADDING];
        for (unsigned int k = 0; k < p; k++) {
            padding_zeroes[k] = 0;
        }

        const bool gamma_enabled = gamma != 1.0f;
        const double invN = 1.0 / n;
        const double inv255 = 1.0 / 255.0;
        // const double inv255invN = invN * inv255;

        for (unsigned int j = 0; j < height; j++) {
            const unsigned int indexj = height - j - 1;
            for (unsigned int i = 0; i < width; i++) {
                const rt::color& c = data[i][indexj];
                if (gamma_enabled) {
                    const unsigned char r = pow((std::min((c.get_red()   * invN), 255.0) * inv255), gamma) * 255.0;
                    const unsigned char g = pow((std::min((c.get_green() * invN), 255.0) * inv255), gamma) * 255.0;
                    const unsigned char b = pow((std::min((c.get_blue()  * invN), 255.0) * inv255), gamma) * 255.0;
                    ret = fprintf(file, "%c%c%c", b, g, r);
                }
                else {
                    const char r = std::min(c.get_red()   * invN, 255.0);
                    const char g = std::min(c.get_green() * invN, 255.0);
                    const char b = std::min(c.get_blue()  * invN, 255.0);
                    ret = fprintf(file, "%c%c%c", b, g, r);
                }
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
