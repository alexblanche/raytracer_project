#include "file_readers/bmp_reader.hpp"

#include "file_readers/file.hpp"

#include <iostream>
#include <stdexcept>
#include <cmath>

static constexpr int BYTES_PER_COLOR = 3;

/* Extracts the data from the given .bmp file into the matrix data, which must have the right size */
std::optional<matrix> read_bmp(const std::string& file_name) {

    file f(file_name, "rb");

    try {
        /* 18 bytes ignored:
           Type (2), Size (4), Reserved 1 (2), Reserved 2 (2), Offset (4), Size of the header (4)
        */
        f.skip(18);
        
        /* Width: 4 bytes */
        unsigned int bmpwidth;
        const exit_status status_w = f.read<unsigned int>({ &bmpwidth, 1 });
        if (status_w == exit_status::Failure)
            throw std::runtime_error("Reading error in read_bmp_size (width)");

        /* Height: 4 bytes */
        unsigned int bmpheight;
        const exit_status status_h = f.read<unsigned int>({ &bmpheight, 1 });
        if (status_h == exit_status::Failure)
            throw std::runtime_error("Reading error in read_bmp_size (height)");

        /* 28 bytes ignored:
           Number of color planes (2), Number of bits per pixel (2), Compression method used (4),
           Compressed size of the image (4), Horizontal resolution (4), Vertical resolution (4),
           Number of colors used (4), Number of important colors used (4)
        */
        f.skip(28);

        /* Padding at the end of each row in the file */
        const unsigned int p = (4 - ((BYTES_PER_COLOR * bmpwidth) % 4)) % 4;

        /* Color data */
        const unsigned int nb_bytes = ((BYTES_PER_COLOR * bmpwidth) + p) * bmpheight;
        std::vector<unsigned char> buffer(nb_bytes);
        const exit_status status = f.read(buffer);
        if (status == exit_status::Failure)
            throw std::runtime_error("Reading error in read_bmp (pixel data)");

        matrix matrix(bmpwidth, bmpheight);

        unsigned int index = 0;
        for (unsigned int j = 0; j < bmpheight; j++) {
            const unsigned int indexj = bmpheight - j - 1;
            for (unsigned int i = 0; i < bmpwidth; i++) {
                const real b = buffer[index];
                const real g = buffer[index + 1];
                const real r = buffer[index + 2];
                matrix.data[i][indexj] = rt::color(r, g, b);
                index += 3;
            }
            index += p;
        }

        return matrix;
    }
    catch(const std::exception& e) {
        printf("%s\n", e.what());
        return std::nullopt;
    }
}

/* Prints the info contained in the header of the given .bmp file */
exit_status print_bmp_info(const std::string& file_name) {

    file f(file_name, "rb");

    // "BM" characters
    char filetype[2];
    f.read<char>(filetype);

    std::optional<bmp_header> h_opt = f.scan<bmp_header>();

    if (not h_opt.has_value()) {
        printf("Reading error in read_bmp header\n");
        return exit_status::Failure;
    }
    
    printf("Type:                  %.2s\n", filetype);
    h_opt.value().print();
    return exit_status::Success;
}


static inline void check(exit_status status) {
    if (status == exit_status::Failure)
        throw std::runtime_error("");
}

/* Writes the data into a .bmp file with the given name
   The value (double) of each component of each color of data is divided by number_of_rays before being written in the file
   Returns true if the operation was successful */
exit_status write_bmp(const std::string& file_name, const image& image) {

    const auto [ width, height ] = image.data.get_dimensions();

    const unsigned int p = (4 - ((BYTES_PER_COLOR * width) % 4)) % 4;
    const bool padding = (p != 0);

    file f(file_name, "wb");

    /* All sizes are stored in little-endian */
    try {
        /** Header **/

        /* 2 bytes: BM */
        check(f.printf("BM"));

        /* 4 bytes: File size */
        /* Size = 14 (header) + 40 (infoheader) + BYTES_PER_COLOR * width * height (pixel data) + p * height */
        const unsigned int file_size = 14 + 40 + (BYTES_PER_COLOR * width + p) * height;
        check(f.write<char>(
            file_size         & 0xFF,
            (file_size >> 8)  & 0xFF,
            (file_size >> 16) & 0xFF,
            (file_size >> 24) & 0xFF
        ));

        /* 4 bytes: 0 0 0 0 */
        check(f.write<char, '\0', 4>());

        /* 4 bytes: Offset from beginning of file to the beginning of the bitmap data = 54 */
        check(f.write<char>(54, 0, 0, 0));

        /** InfoHeader **/

        /* 4 bytes: Size of InfoHeader = 40 */
        check(f.write<char>(40, 0, 0, 0));

        /* 4 bytes: Width */
        check(f.write<char>(
            width         & 0xFF,
            (width >> 8)  & 0xFF,
            (width >> 16) & 0xFF,
            (width >> 24) & 0xFF
        ));

        /* 4 bytes: Height */
        check(f.write<char>(
            height         & 0xFF,
            (height >> 8)  & 0xFF,
            (height >> 16) & 0xFF,
            (height >> 24) & 0xFF
        ));

        /* 2 bytes: Planes = 1 */
        check(f.write<char>(1, 0));

        /* 2 bytes: Bits per Pixel (24 for 24 bits RGB) */
        check(f.write<char>(24, 0));

        /* 4 bytes: compression (0 for no compression) */
        check(f.write<char, '\0', 4>());

        /* 4 bytes: Size of the pixel data */
        /* Size = 3 * width * height (pixel data) + p * height */
        const unsigned int data_size = (BYTES_PER_COLOR * width + p) * height;
        check(f.write<char>(
            data_size         & 0xFF,
            (data_size >> 8)  & 0xFF,
            (data_size >> 16) & 0xFF,
            (data_size >> 24) & 0xFF
        ));

        /* 4 bytes: Horizontal resolution (in pixels/meter)
        Unimportant: leaving it as 0 */
        check(f.write<char, '\0', 4>());

        /* 4 bytes: Vertical resolution (in pixels/meter)
        Unimportant: leaving it as 0 */
        check(f.write<char, '\0', 4>());

        /* 4 bytes: Number of actually used colors
        Unimportant: leaving it as 0 */
        check(f.write<char, '\0', 4>());

        /* 4 bytes: Important colors
        Unimportant: leaving it as 0 */
        check(f.write<char, '\0', 4>());
        

        /** Color data **/
        /* Each pixel is represented as 3 bytes BGR, each line (sequence of 3*width bytes) is followed by p bytes '0' of padding */
        const bool gamma_enabled = image.gamma != 1.0f;
        const real invN = 1.0_r / image.number_of_samples;
        constexpr real inv255 = 1.0_r / 255.0_r;
    
        for (int j = 0; j < height; j++) {
            const int indexj = height - j - 1;
            for (int i = 0; i < width; i++) {
                const rt::color& c = image.data.data[i][indexj];
                
                rt::color col = c * invN;
                col.in_place_max_out();

                if (gamma_enabled) {
                    col *= inv255;
                    col ^= image.gamma;
                    col *= 255.0_r;
                }

                const unsigned char r = col.get_red();
                const unsigned char g = col.get_green();
                const unsigned char b = col.get_blue();
                // const rt::color::uint8_color color_data = corrected.to_uint8_bgr();
                // std::memcpy(buffer + index, &color_data, 3);
                f.write<char>(b, g, r);
            }
            /* Writing p bytes '0' of padding */
            if (padding)
                f.write<char, '\0'>(p);
        }

        return exit_status::Success;
    }
    catch(const std::exception& e) {

        printf("Writing error in file %s\n", file_name.c_str());
        return exit_status::Failure;
    }
}
