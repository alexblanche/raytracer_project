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
            
        // https://www.flipcode.com/archives/HDR_Image_Reader.shtml

        for (unsigned int j = 0; j < height; j++) {
            const unsigned int indexj = j * width;

            const unsigned char b1 = f.getc();
            const unsigned char b2 = f.getc();
            if (b1 != 2 || b2 != 2)
                throw std::runtime_error("Reading error in read_hdr: bytes '2' at beginning of row");
            
            const unsigned int width1 = f.getc();
            const unsigned int width2 = f.getc();
            if ((width1 << 8) + width2 != width)
                throw std::runtime_error("Reading error in read_hdr: width at beginning of row");
            

            for (int component = 0; component < 4; component++) {

                std::vector<unsigned char>& buffer = data_buffer[component];
                //unsigned char* const bufferj = buffer.data() + indexj;
                
                for (unsigned int i = 0; i < width; ) {
                    
                    const unsigned char byte = f.getc();
                    if (byte > 0x80) {
                        // Run
                        unsigned char count = byte & 0x7F;
                        const unsigned char value = f.getc();
                        while (count--) {
                            buffer[indexj + i++] = value;
                        }
                        // std::memset(bufferj + i, fgetc(file), count);
                        // i += count;
                    }
                    else  {
                        // Consecutive distinct bytes
                        unsigned char count = byte;
                        while(count--) {
                            buffer[indexj + i++] = f.getc();
                        }
                        // if (1 != fread(static_cast<void*>(bufferj + i), count, 1, file))
                        //      throw std::runtime_error("Reading error in read_hdr: pixel data");
                        // i += count;
                    }
                }
            }
        }

        f.close();

        matrix data(width, height);

        parallel_for(width, [&] (int i) {
            std::vector<rt::color>& data_line = data.data[i];
            for (unsigned int j = 0; j < height; j++) {
                const unsigned int index = j * width + i;
                data_line[j] = rt::color(
                    data_buffer[0][index],
                    data_buffer[1][index],
                    data_buffer[2][index]
                );
                const int e =  data_buffer[3][index];
                const real radiance_val = pow(2.0f, e - 128);
                data_line[j] *= radiance_val;
            }
        });

        return data;
    }
    catch(const std::exception& e) {
        printf("%s\n", e.what());
        return std::nullopt;
    }
}