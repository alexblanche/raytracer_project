#include "file_readers/image_files/hdr_reader.hpp"

#include "screen/screen.hpp"
#include "parallel/parallel.hpp"
#include "file_readers/file.hpp"

#include <cmath>
#include <stdexcept>
#include <cstring>

using enum file_reader::error;

std::expected<matrix, file_reader::error> hdr::read_file(const std::string& file_name) {

    try {

        file f(file_name, "rb");

        {
            float gamma;
            int p[8];
            char format[16];
            const exit_status status = f.scanf("#?RADIANCE\n#?RADIANCE\nGAMMA=%f\nPRIMARIES=%d %d %d %d %d %d %d %d\nFORMAT=%15s",
                gamma, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], format);
            if (status == exit_status::Failure) {
                const exit_status status_format = f.scanf("FORMAT=%15s", format);
                throw_if_failure(status_format, ReadingErrorHeader);
            }
        }
        
        char s1, s2, l1, l2;
        unsigned int v1, v2;
        {
            const exit_status status = f.scanf("\n%c%c %u %c%c %u\n", s1, l1, v1, s2, l2, v2);
            throw_if_failure(status, DataError);
        }
        if (not ((l1 == 'X' || l1 == 'Y') && (l2 == 'X' || l2 == 'Y')
              && (s1 == '-' || s1 == '+') && (s2 == '-' || s2 == '+')))
            throw DataError;

        const auto [ width, height ] = (l1 == 'X') ? std::pair { v1, v2 } : std::pair { v2, v1 };

        // Filling pixel data
        std::array<std::vector<unsigned char>, 4> data_buffer;
        for (auto& v : data_buffer)
            v.resize(width * height);

        const std::vector<unsigned char> content = f.extract_from();
        const std::size_t length = content.size();
        if (length > static_cast<std::size_t>(INT32_MAX)) {
            printf("read_hdr: file too large (> %d bytes)\n", INT32_MAX);
            throw FileError;
        }

        f.close();
        int pos = 0;
            
        // https://www.flipcode.com/archives/HDR_Image_Reader.shtml

        // 4 chars: 2, 2, (width & 0xFF00), (width & 0x00FF) in order
        const uint32_t expected_row_header = ((width & 0x00FF) << 24) | (((width & 0xFF00) | 2) << 8) | 2;

        for (unsigned int j = 0; j < height; j++) {
            const unsigned int indexj = j * width;

            const uint32_t header = *reinterpret_cast<const uint32_t*>(content.data() + pos);
            if (header != expected_row_header) [[unlikely]] {
                printf("Reading error in read_hdr: bytes '2' or width at beginning of row\n");
                throw DataError;
            }
            pos += 4;
            
            for (int component = 0; component < 4; component++) {

                std::vector<unsigned char>& buffer = data_buffer[component];
                unsigned char* const buffer_j = buffer.data() + indexj;
                
                for (unsigned int i = 0; i < width; ) {
                    
                    const unsigned char byte = content[pos++];
                    unsigned int count;
                    if (byte > 0x80) {
                        // Run
                        count = byte & 0x7F;
                        std::memset(buffer_j + i, content[pos++], count);
                    }
                    else  {
                        // Consecutive distinct bytes
                        count = byte;
                        std::memcpy(buffer_j + i, content.data() + pos, count);
                        pos += count;
                    }
                    i += count;
                }
            }
        }

        matrix data(width, height);

        parallel_for(height, [&] (int j) {

            int index = (height - 1 - j) * width;
            const matrix::row row = data[j];

            for (rt::color& color : row) {

                const rt::color col(
                    data_buffer[0][index],
                    data_buffer[1][index],
                    data_buffer[2][index]
                );

                const int e = data_buffer[3][index];
                const real radiance_val = std::exp2(static_cast<real>(e - 128));

                color = col * radiance_val;

                index++;
            }
        });

        return data;
    }
    catch(file::error e) {
        return std::unexpected(file_reader::error::FileError);
    }
    catch(const std::exception& e) {
        printf("%s\n", e.what());
        return std::unexpected(file_reader::error::Other);
    }
}