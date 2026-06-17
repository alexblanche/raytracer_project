#include "file_readers/image_files/bmp_reader.hpp"

#include "file_readers/file.hpp"

#include "parallel/parallel.hpp"

#include <iostream>
#include <stdexcept>
#include <cmath>

static constexpr int BYTES_PER_COLOR = 3;

/* Extracts the data from the given .bmp file into the matrix data, which must have the right size */
std::optional<matrix> bmp::read_file(const std::string& file_name) {

    try {
        
        file f(file_name, "rb");

        /* 18 bytes ignored:
           Type (2), Size (4), Reserved 1 (2), Reserved 2 (2), Offset (4), Size of the header (4)
        */
        f.skip(18);
        
        /* Width: 4 bytes */
        unsigned int bmpwidth;
        const exit_status status_w = f.read<unsigned int>({ &bmpwidth, 1 });
        throw_if_failure(status_w, "Reading error in read_bmp_size (width)");

        /* Height: 4 bytes */
        unsigned int bmpheight;
        const exit_status status_h = f.read<unsigned int>({ &bmpheight, 1 });
        throw_if_failure(status_h, "Reading error in read_bmp_size (height)");

        /* 28 bytes ignored:
           Number of color planes (2), Number of bits per pixel (2), Compression method used (4),
           Compressed size of the image (4), Horizontal resolution (4), Vertical resolution (4),
           Number of colors used (4), Number of important colors used (4)
        */
        f.skip(28);

        const unsigned int row_length_bytes = BYTES_PER_COLOR * bmpwidth;

        /* Padding at the end of each row in the file */
        const unsigned int padding_bytes = (4 - (row_length_bytes % 4)) % 4;

        /* Color data */
        const unsigned int size_bytes = (row_length_bytes + padding_bytes) * bmpheight;
        std::vector<unsigned char> buffer(size_bytes);
        const exit_status status = f.read(buffer);
        throw_if_failure(status, "Reading error in read_bmp (pixel data)");
        f.close();

        matrix matrix(bmpwidth, bmpheight);

        parallel_for(bmpheight, [&] (int j) {

            const matrix::row row = matrix[j];
            unsigned int index = 3 * j * (bmpwidth + padding_bytes);
            for (rt::color& color : row) {
                const real b = buffer[index];
                const real g = buffer[index + 1];
                const real r = buffer[index + 2];
                color = rt::color(r, g, b);
                index += 3;
            }
        });

        return matrix;
    }
    catch(const std::exception& e) {
        printf("%s\n", e.what());
        return std::nullopt;
    }
}

/* Prints the info contained in the header of the given .bmp file */
exit_status bmp::print_info(const std::string& file_name) {

    file f(file_name, "rb");

    // "BM" characters
    char filetype[2];
    f.read<char>(filetype);

    std::optional<bmp::header> h_opt = f.scan<bmp::header>();

    if (not h_opt.has_value()) {
        printf("Reading error in read_bmp header\n");
        return exit_status::Failure;
    }
    
    printf("Type:                  %.2s\n", filetype);
    h_opt.value().print();
    return exit_status::Success;
}


static inline void check(exit_status status) {
    throw_if_failure(status, "");
}

struct byte_representation {

    uint8_t b0, b1, b2, b3;

    byte_representation(unsigned int n) :
        b0( n        & 0xFF),
        b1((n >> 8)  & 0xFF),
        b2((n >> 16) & 0xFF),
        b3((n >> 24) & 0xFF) {}
};

/* Writes the data into a .bmp file with the given name
   The value (double) of each component of each color of data is divided by number_of_rays before being written in the file
   Returns true if the operation was successful */
exit_status bmp::export_data(const std::string& file_name, const image& image) {

    const auto [ width, height ] = image.data.get_dimensions();

    const unsigned int row_length_bytes = BYTES_PER_COLOR * width;
    const unsigned int padding_bytes = (4 - (row_length_bytes % 4)) % 4;

    /* All sizes are stored in little-endian */

    const unsigned int data_size = (row_length_bytes + padding_bytes) * height;
    const unsigned int file_size = 14 + 40 + data_size;

    /** Header **/

    const auto [ s0, s1, s2, s3 ] = byte_representation(file_size);
    const auto [ w0, w1, w2, w3 ] = byte_representation(width);
    const auto [ h0, h1, h2, h3 ] = byte_representation(height);
    const auto [ d0, d1, d2, d3 ] = byte_representation(data_size);

    uint8_t header[] = {

        /* 2 bytes: BM */
        'B', 'M', 

        /* 4 bytes: File size */
        s0, s1, s2, s3,

        /* 4 bytes: 0 0 0 0 */
        0, 0, 0, 0,
        
        /* 4 bytes: Offset from beginning of file to the beginning of the bitmap data = 54 */
        54, 0, 0, 0,

        /** InfoHeader **/

        /* 4 bytes: Size of InfoHeader = 40 */
        40, 0, 0, 0,
        
        /* 4 bytes: Width */
        w0, w1, w2, w3,
        
        /* 4 bytes: Height */
        h0, h1, h2, h3,

        /* 2 bytes: Planes = 1 */
        1,  0,

        /* 2 bytes: Bits per Pixel (24 for 24 bits RGB) */
        24, 0,

        /* 4 bytes: compression (0 for no compression) */
        0, 0, 0, 0,

        /* 4 bytes: Size of the pixel data */
        d0, d1, d2, d3,

        /*
            16 unimportant bytes left as 0
            - 4 bytes: Horizontal resolution (in pixels/meter)
            - 4 bytes: Vertical resolution (in pixels/meter)
            - 4 bytes: Number of actually used colors
            - 4 bytes: Important colors
        */
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    };

    /** Color data **/

    std::vector<uint8_t> buffer(data_size, 0);

    /* Each pixel is represented as 3 bytes BGR, each line (sequence of 3*width bytes) is followed by p bytes '0' of padding */
    const bool gamma_enabled = image.gamma != 1.0_r;
    const real invN = 1.0_r / image.number_of_samples;

    int index = 0;
    for (const matrix::const_row row : image.data) {
        for (const rt::color& c : row) {
            
            rt::color col = c * invN;
            col.cap();

            if (gamma_enabled)
                col.apply_gamma(image.gamma);

            const auto [ b, g, r ] = col.to_uint8_bgr();
            buffer[index]     = b;
            buffer[index + 1] = g;
            buffer[index + 2] = r;
            index += 3;
        }
        index += padding_bytes;
    }

    try {

        file f(file_name, "wb");

        check(f.write<uint8_t>(header));
        check(f.write(buffer));

        return exit_status::Success;
    }
    catch(const std::exception& e) {

        printf("Writing error in file %s\n", file_name.c_str());
        return exit_status::Failure;
    }
}
