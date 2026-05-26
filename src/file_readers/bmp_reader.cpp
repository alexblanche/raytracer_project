#include "file_readers/bmp_reader.hpp"
#include "screen/color.hpp"

#include <cstdio>
#include <stdexcept>
#include <cmath>

static constexpr int MAX_PADDING = 4;
static constexpr int BYTES_PER_COLOR = 3;

/* Writes in the variables the width and height of the .bmp image contained in file_name */
std::optional<dimensions> read_bmp_size(const char* file_name) {
    FILE* file = fopen(file_name, "rb");

    if (file == nullptr) {
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
        if (ret != 1)
            throw std::runtime_error("Reading error in read_bmp_size (width)");
        
        const int width = static_cast<int>(bmpwidth);

        /* Height: 4 bytes */
        unsigned int bmpheight;
        ret = fread((void*) &bmpheight, sizeof(int), 1, file);
        if (ret != 1)
            throw std::runtime_error("Reading error in read_bmp_size (height)");
        
        const int height = static_cast<int>(bmpheight);

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
exit_status read_bmp(const char* file_name, std::vector<std::vector<rt::color>>& data) {
    FILE* file = fopen(file_name, "rb");

    if (file == nullptr) {
        printf("Error, file %s not found\n", file_name);
        return exit_status::Failure;
    }

    try {

        constexpr int MAX_LENGTH = 28;
        char buffer_ignore[MAX_LENGTH];
        int ret;

        /* 18 bytes ignored:
        Type (2), Size (4), Reserved 1 (2), Reserved 2 (2), Offset (4), Size of the header (4)
        */
        ret = fread(static_cast<void*>(buffer_ignore), 18, 1, file);
        if (ret != 1)
            throw std::runtime_error("Reading error in read_bmp (18 first bytes)");
        
        /* Width: 4 bytes */
        unsigned int bmpwidth;
        ret = fread(static_cast<void*>(&bmpwidth), sizeof(int), 1, file);
        if (ret != 1)
            throw std::runtime_error("Reading error in read_bmp (width)");

        /* Height: 4 bytes */
        unsigned int bmpheight;
        ret = fread(static_cast<void*>(&bmpheight), sizeof(int), 1, file);
        if (ret != 1)
            throw std::runtime_error("Reading error in read_bmp (height)");

        /* 28 bytes ignored:
        Number of color planes (2), Number of bits per pixel (2), Compression method used (4),
        Compressed size of the image (4), Horizontal resolution (4), Vertical resolution (4),
        Number of colors used (4), Number of important colors used (4)
        */
        ret = fread(static_cast<void*>(buffer_ignore), 28, 1, file);
        if (ret != 1)
            throw std::runtime_error("Reading error in read_bmp_size (28 bytes after height)");
        
        /* Padding at the end of each row in the file */
        const unsigned int p = (4 - ((BYTES_PER_COLOR * bmpwidth) % 4)) % 4;

        /* Color data */
        const unsigned int nb_bytes = ((BYTES_PER_COLOR * bmpwidth) + p) * bmpheight;
        std::vector<unsigned char> buffer(nb_bytes);
        ret = fread((void*) buffer.data(), 1, nb_bytes, file);
        if (static_cast<unsigned int>(ret) != nb_bytes)
            throw std::runtime_error("Reading error in read_bmp (pixel data)");

        unsigned int index = 0;
        for (unsigned int j = 0; j < bmpheight; j++) {
            const unsigned int indexj = bmpheight - j - 1;
            for (unsigned int i = 0; i < bmpwidth; i++) {
                const real b = buffer[index];
                const real g = buffer[index + 1];
                const real r = buffer[index + 2];
                data[i][indexj] = rt::color(r, g, b);
                index += 3;
            }
            index += p;
        }

        fclose(file);
        return exit_status::Success;
    }
    catch(const std::exception& e) {
        printf("%s\n", e.what());
        fclose(file);
        return exit_status::Failure;
    }
}

/* Returns the integer stored in the buffer at index start_index on nb_bytes bytes (in Little Endian convention) */
template<int nb_bytes>
static inline unsigned int value_of_bytes(const unsigned char buffer[], const int start_index) {
    if constexpr (nb_bytes == 4) {
        const uint32_t value = * reinterpret_cast<const uint32_t*>(buffer + start_index);
        return value;
    }
    else {
        const uint16_t value = * reinterpret_cast<const uint16_t*>(buffer + start_index);
        return static_cast<unsigned int>(value);
    }
}

/* Prints the info contained in the header of the given .bmp file */
exit_status print_bmp_info(const char* file_name) {
    FILE* file = fopen(file_name, "rb");

    if (file == nullptr) {
        printf("Error, file %s not found\n", file_name);
        return exit_status::Failure;
    }

    try {

        int ret;

        constexpr unsigned int HEADER_SIZE = 54;
        unsigned char buffer[HEADER_SIZE];
        ret = fread((void*) buffer, HEADER_SIZE, 1, file);
        if (ret != 1)
            throw std::runtime_error("Reading error in read_bmp");

        printf("Type:                  %c%c\n",     buffer[0], buffer[1]);
        printf("File size:             %u bytes\n", value_of_bytes<4>(buffer, 2));
        printf("Reserved 1:            0x%x%x\n",   buffer[6], buffer[7]);
        printf("Reserved 2:            0x%x%x\n",   buffer[8], buffer[9]);
        printf("Offset:                %u\n",       value_of_bytes<4>(buffer, 10));
        printf("Header size:           %u bytes\n", value_of_bytes<4>(buffer, 14));
        printf("Width:                 %u\n",       value_of_bytes<4>(buffer, 18));
        printf("Height:                %u\n",       value_of_bytes<4>(buffer, 22));
        printf("Color planes:          %u\n",       value_of_bytes<2>(buffer, 26));
        printf("Bits per pixel:        %u\n",       value_of_bytes<2>(buffer, 28));
        printf("Compression method:    %u\n",       value_of_bytes<4>(buffer, 30));
        printf("Compressed size:       %u bytes\n", value_of_bytes<4>(buffer, 34));
        printf("Horizontal resolution: %u\n",       value_of_bytes<4>(buffer, 38));
        printf("Vertical resolution:   %u\n",       value_of_bytes<4>(buffer, 42));
        printf("Colors used:           %u\n",       value_of_bytes<4>(buffer, 46));
        printf("Important colors:      %u\n",       value_of_bytes<4>(buffer, 50));

        fclose(file);
        return exit_status::Success;
    }
    catch(const std::exception& e) {
        printf("%s\n", e.what());
        fclose(file);
        return exit_status::Failure;
    }
}

inline void throw_if_error(int ret) {
    if (ret < 0)
        throw std::runtime_error("");
}

/* Writes the data into a .bmp file with the given name
   The value (double) of each component of each color of data is divided by number_of_rays before being written in the file
   Returns true if the operation was successful */
exit_status write_bmp(const char* file_name, const std::vector<std::vector<rt::color>>& data,
    const unsigned int number_of_rays, const real gamma) {

    const int width  = data.size();
    const int height = data[0].size();

    const unsigned int p = (4 - ((BYTES_PER_COLOR * width) % 4)) % 4;
    const bool padding = (p != 0);

    FILE* file = fopen(file_name, "wb");

    if (file == nullptr) {
        printf("Error, file %s not found\n", file_name);
        return exit_status::Failure;
    }

    /* All sizes are stored in little-endian */
    try {
        int ret;

        /** Header **/

        /* 2 bytes: BM */
        ret = fprintf(file, "BM");
        throw_if_error(ret);

        /* 4 bytes: File size */
        /* Size = 14 (header) + 40 (infoheader) + BYTES_PER_COLOR * width * height (pixel data) + p * height */
        const unsigned int file_size = 14 + 40 + (BYTES_PER_COLOR * width + p) * height;
        ret = fprintf(file, "%c%c%c%c",
            file_size         & 0xFF,
            (file_size >> 8)  & 0xFF,
            (file_size >> 16) & 0xFF,
            (file_size >> 24) & 0xFF
        );
        throw_if_error(ret);

        /* 4 bytes: 0 0 0 0 */
        ret = fprintf(file, "%c%c%c%c", 0, 0, 0, 0);
        throw_if_error(ret);

        /* 4 bytes: Offset from beginning of file to the beginning of the bitmap data = 54 */
        ret = fprintf(file, "%c%c%c%c", 54, 0, 0, 0);
        throw_if_error(ret);

        /** InfoHeader **/

        /* 4 bytes: Size of InfoHeader = 40 */
        ret = fprintf(file, "%c%c%c%c", 40, 0, 0, 0);
        throw_if_error(ret);

        /* 4 bytes: Width */
        ret = fprintf(file, "%c%c%c%c",
            width         & 0xFF,
            (width >> 8)  & 0xFF,
            (width >> 16) & 0xFF,
            (width >> 24) & 0xFF
        );
        throw_if_error(ret);

        /* 4 bytes: Height */
        ret = fprintf(file, "%c%c%c%c",
            height         & 0xFF,
            (height >> 8)  & 0xFF,
            (height >> 16) & 0xFF,
            (height >> 24) & 0xFF
        );
        throw_if_error(ret);

        /* 2 bytes: Planes = 1 */
        ret = fprintf(file, "%c%c", 1, 0);
        throw_if_error(ret);

        /* 2 bytes: Bits per Pixel (24 for 24 bits RGB) */
        ret = fprintf(file, "%c%c", 24, 0);
        throw_if_error(ret);

        /* 4 bytes: compression (0 for no compression) */
        ret = fprintf(file, "%c%c%c%c", 0, 0, 0, 0);
        throw_if_error(ret);

        /* 4 bytes: Size of the pixel data */
        /* Size = 3 * width * height (pixel data) + p * height */
        const unsigned int data_size = (BYTES_PER_COLOR * width + p) * height;
        ret = fprintf(file, "%c%c%c%c",
            data_size         & 0xFF,
            (data_size >> 8)  & 0xFF,
            (data_size >> 16) & 0xFF,
            (data_size >> 24) & 0xFF
        );
        throw_if_error(ret);

        /* 4 bytes: Horizontal resolution (in pixels/meter)
        Unimportant: leaving it as 0 */
        ret = fprintf(file, "%c%c%c%c", 0, 0, 0, 0);
        throw_if_error(ret);

        /* 4 bytes: Vertical resolution (in pixels/meter)
        Unimportant: leaving it as 0 */
        ret = fprintf(file, "%c%c%c%c", 0, 0, 0, 0);
        throw_if_error(ret);

        /* 4 bytes: Number of actually used colors
        Unimportant: leaving it as 0 */
        ret = fprintf(file, "%c%c%c%c", 0, 0, 0, 0);
        throw_if_error(ret);

        /* 4 bytes: Important colors
        Unimportant: leaving it as 0 */
        ret = fprintf(file, "%c%c%c%c", 0, 0, 0, 0);
        throw_if_error(ret);
        

        /** Color data **/
        /* Each pixel is represented as 3 bytes BGR, each line (sequence of 3*width bytes) is followed by p bytes '0' of padding */

        /* Padding bytes at the end of each line (if padding = true) */
        char padding_zeroes[MAX_PADDING] = { 0 };

        const bool gamma_enabled = gamma != 1.0f;
        const real invN = 1.0 / number_of_rays;
        constexpr real inv255 = 1.0 / 255.0;
    
        for (int j = 0; j < height; j++) {
            const int indexj = height - j - 1;
            for (int i = 0; i < width; i++) {
                const rt::color& c = data[i][indexj];
                
                rt::color col = c * invN;
                col.in_place_max_out();

                if (gamma_enabled) {
                    col *= inv255;
                    col ^= gamma;
                    col *= static_cast<real>(255.0f);
                }

                const unsigned char r = col.get_red();
                const unsigned char g = col.get_green();
                const unsigned char b = col.get_blue();
                // const rt::color::uint8_color color_data = corrected.to_uint8_bgr();
                // std::memcpy(buffer + index, &color_data, 3);
                const int ret = fprintf(file, "%c%c%c", b, g, r);
                throw_if_error(ret);
            }
            /* Writing p bytes '0' of padding */
            if (padding) {
                const int ret = fwrite((void*) padding_zeroes, p, 1, file);
                if (ret != 1)
                    throw std::runtime_error("");
            }
        }

        fclose(file);
        return exit_status::Success;
    }
    catch(const std::exception& e) {
        fclose(file);
        printf("Writing error in file %s\n", file_name);
        return exit_status::Failure;
    }
}
