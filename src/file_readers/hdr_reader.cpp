#include "file_readers/hdr_reader.hpp"

#include "screen/screen.hpp"
#include "parallel/parallel.hpp"
#include "file_readers/file.hpp"

#include <cmath>
#include <stdexcept>

std::optional<matrix> read_hdr(const std::string& file_name) {

    file f(file_name, "rb");

    try {

        float gamma;
        int p1, p2, p3, p4, p5, p6, p7, p8;
        char format[16];
        {
            const exit_status status = f.scanf("#?RADIANCE\n#?RADIANCE\nGAMMA=%f\nPRIMARIES=%d %d %d %d %d %d %d %d\nFORMAT=%15s",
                gamma, p1, p2, p3, p4, p5, p6, p7, p8, format);
            if (status == exit_status::Failure) {
                const exit_status status = f.scanf("FORMAT=%15s", format);
                if (status == exit_status::Failure)
                    throw std::runtime_error("Reading error in print_hdr_info: header");
            }
        }
        
        char s1, s2, l1, l2;
        unsigned int v1, v2;
        {
            const exit_status status = f.scanf("\n%c%c %u %c%c %u\n", s1, l1, v1, s2, l2, v2);
            if (status == exit_status::Failure)
                throw std::runtime_error("Reading error in print_hdr_info: dimensions");
        }
        if ((l1 != 'X' && l1 != 'Y') || (l2 != 'X' && l2 != 'Y') || (s1 != '-' && s1 != '+') || (s2 != '-' && s2 != '+'))
            throw std::runtime_error("Incorrect dimensions");

        bool l1_is_x = l1 == 'X';
        // bool x_orientation  = l1_is_x ? s1 == '+' : s2 == '+';
        // bool y_orientation  = l1_is_x ? s2 == '+' : s1 == '+';
        unsigned int width  = l1_is_x ? v1 : v2;
        unsigned int height = l1_is_x ? v2 : v1;

        // Filling pixel data
        std::array<std::vector<unsigned char>, 4> data_buffer;
        for (std::vector<unsigned char>& v : data_buffer)
            v.resize(width * height);

        const std::vector<unsigned char> content = f.extract_from();
        const std::size_t length = f.length();
        if (length > static_cast<std::size_t>(INT32_MAX)) {
            throw std::runtime_error("read_hdr: file too large");
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
                throw std::runtime_error("Reading error in read_hdr: bytes '2' or width at beginning of row");
            }
            pos++;
            
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
                color = rt::color(
                    data_buffer[0][index],
                    data_buffer[1][index],
                    data_buffer[2][index]
                );
                const int e =  data_buffer[3][index];
                const real radiance_val = pow(2.0_r, e - 128);
                color *= radiance_val;
                index ++;
            }
        });

        return data;
    }
    catch(const std::exception& e) {
        printf("%s\n", e.what());
        return std::nullopt;
    }
}