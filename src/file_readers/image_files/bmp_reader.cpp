#include "file_readers/image_files/bmp_reader.hpp"

#include "file_readers/file.hpp"

#include "parallel/parallel.hpp"

#include <iostream>
#include <stdexcept>

static constexpr int DEFAULT_BYTES_PER_COLOR = 3;

using enum file_reader::error;

/* Extracts the data from the given .bmp file into the matrix data, which must have the right size */
std::expected<matrix, file_reader::error> bmp::read_file(const std::string& file_name) {

    try {
        
        file f(file_name, "rb");

        // "BM" characters
        const std::string file_type = f.read_string(2);
        if (file_type != "BM")
            throw DataError;

        const std::optional<bmp::header> h_opt = f.scan<bmp::header>();

        if (not h_opt.has_value())
            throw ReadingErrorHeader;

        const auto& [
            _ /* file_size */,
            _ /* reserved1 */,
            _ /* reserved2 */,
            _ /* offset */,
            _ /* header_size */,
            width,
            height,
            _ /* color_planes */,
            bits_per_pixel,
            _ /* compression_method */,
            _ /* compressed_size */,
            _ /* horizontal_resolution */,
            _ /* vertical_resolution */,
            _ /* colors_used */,
            _ /* important_colors */
        ] = h_opt.value();

        const unsigned int bytes_per_pixels = bits_per_pixel / 8;
        const unsigned int row_length_bytes = bytes_per_pixels * width;

        /* Padding at the end of each row in the file */
        const unsigned int padding_bytes = (4 - (row_length_bytes % 4)) % 4;

        /* Color data */
        const unsigned int size_bytes = (row_length_bytes + padding_bytes) * height;
        std::vector<unsigned char> buffer(size_bytes);
        const exit_status status = f.read(buffer);
        throw_if_failure(status, ReadingErrorData);
        f.close();

        matrix matrix(width, height);

        parallel_for(height, [&] (int j) {

            const matrix::row row = matrix[j];
            unsigned int index = bytes_per_pixels * j * (width + padding_bytes);
            for (rt::color& color : row) {
                const real b = buffer[index];
                const real g = buffer[index + 1];
                const real r = buffer[index + 2];
                color = rt::color(r, g, b);
                index += bytes_per_pixels;
            }
        });

        return matrix;
    }
    catch(file::error) {
        return std::unexpected(FileError);
    }
    catch(file_reader::error e) {
        return std::unexpected(e);
    }
    catch(const std::exception& e) {
        printf("%s\n", e.what());
        return std::unexpected(Other);
    }
}

/* Prints the info contained in the header of the given .bmp file */
exit_status bmp::print_info(const std::string& file_name) {

    file f(file_name, "rb");

    // "BM" characters
    const std::string filetype = f.read_string(2);

    std::optional<bmp::header> h_opt = f.scan<bmp::header>();

    if (not h_opt.has_value()) {
        printf("Reading error in read_bmp header\n");
        return exit_status::Failure;
    }
    
    printf("Type:                  %s\n", filetype.c_str());
    h_opt.value().print();
    return exit_status::Success;
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

    const unsigned int row_length_bytes = DEFAULT_BYTES_PER_COLOR * width;
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
        8 * DEFAULT_BYTES_PER_COLOR, 0,

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
    const bool gamma_enabled = image.gamma.has_value();
    const real invN = 1.0_r / image.number_of_samples;

    int index = 0;
    for (const matrix::const_row row : image.data) {
        for (const rt::color& c : row) {
            
            rt::color col = c * invN;
            col.cap();

            if (gamma_enabled)
                col.apply_gamma(image.gamma.value());

            const auto [ b, g, r ] = col.to_uint8_bgr();
            buffer[index]     = b;
            buffer[index + 1] = g;
            buffer[index + 2] = r;
            if constexpr (DEFAULT_BYTES_PER_COLOR == 4) {
                buffer[index + 3] = 255;
            }
            index += DEFAULT_BYTES_PER_COLOR;
        }
        index += padding_bytes;
    }

    try {

        file f(file_name, "wb");

        using enum file_reader::error;
        throw_if_failure(f.write<uint8_t>(header), WritingErrorHeader);
        throw_if_failure(f.write(buffer),          WritingErrorData);
        
        return exit_status::Success;
    }
    catch(...) {

        printf("Writing error in file %s\n", file_name.c_str());
        return exit_status::Failure;
    }
}
