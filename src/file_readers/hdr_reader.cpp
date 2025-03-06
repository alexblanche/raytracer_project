#include "file_readers/hdr_reader.hpp"
#include "screen/color.hpp"
#include "screen/screen.hpp"

#include <stdio.h>
#include <string.h>
#include <cstdint>

#include <cmath>
#include <stdexcept>

#include "parallel/parallel.h"

// Parallel for-loop macros
#define PARALLEL_FOR_BEGIN(nb_elements) parallel_for(nb_elements, [&](int start, int end){ for(int i = start; i < end; ++i)
#define PARALLEL_FOR_END()})

/* Prints the info contained in the header of the given .hdr file */
bool print_hdr_info(const char* file_name) {
    FILE* file = fopen(file_name, "rb");

    if (file == NULL) {
        printf("Error, file %s not found\n", file_name);
        return false;
    }

    try {

        int ret;

        float gamma;
        int p1, p2, p3, p4, p5, p6, p7, p8;
        char format[16];
        ret = fscanf(file, "#?RADIANCE\n#?RADIANCE\nGAMMA=%f\nPRIMARIES=%d %d %d %d %d %d %d %d\nFORMAT=%15s", &gamma, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, format);
        if (ret < 10) {
            ret = fscanf(file, "FORMAT=%15s", format);
            if (ret < 0) {
                throw std::runtime_error("Readding error in print_hdr_info: header");
            }
        }
        else {
            printf("Gamma:      %f\n", gamma);
            printf("Primaries:  %d %d %d %d %d %d %d %d\n", p1, p2, p3, p4, p5, p6, p7, p8);
        }
        
        printf("Format:     %s\n", format);

        char s1, s2, l1, l2;
        unsigned int v1, v2;
        ret = fscanf(file, "\n%c%c %u %c%c %u\n", &s1, &l1, &v1, &s2, &l2, &v2);
        if (ret < 6) {
            throw std::runtime_error("Readding error in print_hdr_info: dimensions");
        }
        if ((l1 != 'X' && l1 != 'Y') || (l2 != 'X' && l2 != 'Y') || (s1 != '-' && s1 != '+') || (s2 != '-' && s2 != '+')) {
            throw std::runtime_error("Incorrect dimensions");
        }

        bool l1_is_x = l1 == 'X';
        bool x_orientation  = l1_is_x ? s1 == '+' : s2 == '+';
        bool y_orientation  = l1_is_x ? s2 == '+' : s1 == '+';
        unsigned int width  = l1_is_x ? v1 : v2;
        unsigned int height = l1_is_x ? v2 : v1;

        if (x_orientation)
            printf("Width:      %u\n", width);
        else
            printf("Width:      %u (inverted)\n", width);
        if (y_orientation)
            printf("Height:     %u\n", height);
        else
            printf("Height:     %u (inverted)\n", height);


        // Filling pixel data (test)
        std::vector<unsigned char[4]> data(width * height);
        // https://www.flipcode.com/archives/HDR_Image_Reader.shtml

        // bool run_active = false;
        // unsigned int run_remaining = 0;
        rt::color current_pixel_color(0, 0, 0);

        for (unsigned int j = 0; j < height; j++) {
            // const unsigned int indexj = (y_orientation) ? j : height - j - 1;
            const unsigned int indexj = j * width;

            const unsigned char b1 = fgetc(file);
            const unsigned char b2 = fgetc(file);
            if (b1 != 2 || b2 != 2) {
                throw std::runtime_error("Readding error in print_hdr_info: bytes '2' at beginning of row");
            }
            const unsigned int width1 = fgetc(file);
            const unsigned int width2 = fgetc(file);
            if ((width1 << 8) + width2 != width) {
                throw std::runtime_error("Readding error in print_hdr_info: width at beginning of row");
            }

            for (int component = 0; component < 4; component++) {

                for (unsigned int i = 0; i < width; ) {
                    
                    const unsigned char byte = fgetc(file);
                    if (byte > 0x80) {
                        // Run
                        unsigned char count = byte & 0x7F;
                        const unsigned char value = fgetc(file);
                        while (count--) {
                            data[indexj + i++][component] = value;
                        } 
                    }
                    else  {
                        // Consecutive distinct bytes
                        unsigned char count = byte;
                        while(count--) {
                            data[indexj + i++][component] = fgetc(file);
                        }
                    }
                }
            }
        }

        std::vector<std::vector<rt::color>> matrix(width, std::vector<rt::color>(height));
        // Copy data to matrix

        // Simple Gamma correction
        /*
        for (unsigned int j = 0; j < height; j++) {
            const unsigned int indexj = j * width;
            for (unsigned int i = 0; i < width; i++) {
                unsigned char* const pixel_color = data[indexj + i];
                const real r = pixel_color[0];
                const real g = pixel_color[1];
                const real b = pixel_color[2];
                const unsigned char e = pixel_color[3];
                const real radiance_val = pow(2.0f, e - 127);
                constexpr float gamma_corr = 1.0f / 1.7f;
                matrix[i][j] =
                    rt::color(
                        pow(r * radiance_val / 255.0f, gamma_corr) * 255.0f,
                        pow(g * radiance_val / 255.0f, gamma_corr) * 255.0f,
                        pow(b * radiance_val / 255.0f, gamma_corr) * 255.0f);
            }
        }
        */

        // Extended Reinhardt tone mapping
        float max_luminance = 0.0f;
        for (unsigned int j = 0; j < height; j++) {
            const unsigned int indexj = j * width;
            for (unsigned int i = 0; i < width; i++) {
                unsigned char* const pixel_color = data[indexj + i];
                const real r = pixel_color[0];
                const real g = pixel_color[1];
                const real b = pixel_color[2];
                const unsigned char e = pixel_color[3];
                const real radiance_val = pow(2.0f, e - 127);
                const float lr = r * radiance_val;
                const float lg = g * radiance_val;
                const float lb = b * radiance_val;
                const float luminance = 0.2126 * lr + 0.7152 * lg + 0.0722 * lb;
                if (luminance > max_luminance)
                    max_luminance = luminance;
            }
        }
        const float lwhitecorr = 1.0f / (max_luminance * max_luminance);
        printf("\nmax luminance %f; lwhitecorr %f\n", max_luminance, lwhitecorr);

        for (unsigned int j = 0; j < height; j++) {
            const unsigned int indexj = j * width;
            for (unsigned int i = 0; i < width; i++) {
                unsigned char* const pixel_color = data[indexj + i];
                const real r = pixel_color[0];
                const real g = pixel_color[1];
                const real b = pixel_color[2];
                const unsigned char e = pixel_color[3];
                const real radiance_val = pow(2.0f, e - 127);
                
                float lr = r * radiance_val;
                float lg = g * radiance_val;
                float lb = b * radiance_val;
                float lin = (0.2126 * lr + 0.7152 * lg + 0.0722 * lb) / 255.0f;
                float lcorr = (1.0f + lin * lwhitecorr) / (1.0f + lin);
                float cr = lr * lcorr;
                float cg = lg * lcorr;
                float cb = lb * lcorr;

                constexpr float gamma_corr = 1.0f / 1.9f;
                matrix[i][j] =
                    rt::color(
                        pow(cr / 255.0f, gamma_corr) * 255.0f,
                        pow(cg / 255.0f, gamma_corr) * 255.0f,
                        pow(cb / 255.0f, gamma_corr) * 255.0f);
            }
        }

        printf("Parsing done, generating bmp...\n");
        write_bmp("../output/hdr_test.bmp", matrix, 1);

        fclose(file);
        return true;
    }
    catch(const std::exception& e) {
        printf("%s\n", e.what());
        fclose(file);
        return false;
    }
}

std::optional<dimensions> read_hdr_size(const char* file_name) {

    FILE* file = fopen(file_name, "rb");

    if (file == NULL) {
        printf("Error, file %s not found\n", file_name);
        return std::nullopt;
    }

    try {

        int ret;

        float gamma;
        int p1, p2, p3, p4, p5, p6, p7, p8;
        char format[16];
        ret = fscanf(file, "#?RADIANCE\n#?RADIANCE\nGAMMA=%f\nPRIMARIES=%d %d %d %d %d %d %d %d\nFORMAT=%15s", &gamma, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, format);
        if (ret < 10) {
            ret = fscanf(file, "FORMAT=%15s", format);
            if (ret < 0) {
                throw std::runtime_error("Reading error in print_hdr_info: header");
            }
        }

        char s1, s2, l1, l2;
        unsigned int v1, v2;
        ret = fscanf(file, "\n%c%c %u %c%c %u\n", &s1, &l1, &v1, &s2, &l2, &v2);
        if (ret < 6) {
            throw std::runtime_error("Reading error in print_hdr_info: dimensions");
        }
        if ((l1 != 'X' && l1 != 'Y') || (l2 != 'X' && l2 != 'Y') || (s1 != '-' && s1 != '+') || (s2 != '-' && s2 != '+')) {
            throw std::runtime_error("Incorrect dimensions");
        }

        bool l1_is_x = l1 == 'X';
        // bool x_orientation  = l1_is_x ? s1 == '+' : s2 == '+';
        // bool y_orientation  = l1_is_x ? s2 == '+' : s1 == '+';
        unsigned int width  = l1_is_x ? v1 : v2;
        unsigned int height = l1_is_x ? v2 : v1;

        return dimensions(width, height);
    }
    catch(const std::exception& e) {
        printf("%s\n", e.what());
        fclose(file);
        return std::nullopt;
    }
}

bool read_hdr(const char* file_name, std::vector<std::vector<rt::color>>& data) {

    FILE* file = fopen(file_name, "rb");

    if (file == NULL) {
        printf("Error, file %s not found\n", file_name);
        return false;
    }

    try {

        int ret;

        float gamma;
        int p1, p2, p3, p4, p5, p6, p7, p8;
        char format[16];
        ret = fscanf(file, "#?RADIANCE\n#?RADIANCE\nGAMMA=%f\nPRIMARIES=%d %d %d %d %d %d %d %d\nFORMAT=%15s", &gamma, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, format);
        if (ret < 10) {
            ret = fscanf(file, "FORMAT=%15s", format);
            if (ret < 0) {
                throw std::runtime_error("Readding error in read_hdr: header");
            }
        }

        char s1, s2, l1, l2;
        unsigned int v1, v2;
        ret = fscanf(file, "\n%c%c %u %c%c %u\n", &s1, &l1, &v1, &s2, &l2, &v2);
        if (ret < 6) {
            throw std::runtime_error("Readding error in print_hdr_info: dimensions");
        }
        if ((l1 != 'X' && l1 != 'Y') || (l2 != 'X' && l2 != 'Y') || (s1 != '-' && s1 != '+') || (s2 != '-' && s2 != '+')) {
            throw std::runtime_error("Incorrect dimensions");
        }

        bool l1_is_x = l1 == 'X';
        // bool x_orientation  = l1_is_x ? s1 == '+' : s2 == '+';
        // bool y_orientation  = l1_is_x ? s2 == '+' : s1 == '+';
        unsigned int width  = l1_is_x ? v1 : v2;
        unsigned int height = l1_is_x ? v2 : v1;

        // Filling pixel data
        std::vector<unsigned char[4]> data_buffer(width * height);
        // https://www.flipcode.com/archives/HDR_Image_Reader.shtml

        rt::color current_pixel_color(0, 0, 0);

        for (unsigned int j = 0; j < height; j++) {
            const unsigned int indexj = j * width;

            const unsigned char b1 = fgetc(file);
            const unsigned char b2 = fgetc(file);
            if (b1 != 2 || b2 != 2) {
                throw std::runtime_error("Readding error in read_hdr: bytes '2' at beginning of row");
            }
            const unsigned int width1 = fgetc(file);
            const unsigned int width2 = fgetc(file);
            if ((width1 << 8) + width2 != width) {
                throw std::runtime_error("Readding error in read_hdr: width at beginning of row");
            }

            for (int component = 0; component < 4; component++) {

                for (unsigned int i = 0; i < width; ) {
                    
                    const unsigned char byte = fgetc(file);
                    if (byte > 0x80) {
                        // Run
                        unsigned char count = byte & 0x7F;
                        const unsigned char value = fgetc(file);
                        while (count--) {
                            data_buffer[indexj + i++][component] = value;
                        } 
                    }
                    else  {
                        // Consecutive distinct bytes
                        unsigned char count = byte;
                        while(count--) {
                            data_buffer[indexj + i++][component] = fgetc(file);
                        }
                    }
                }
            }
        }

        // Copy data to matrix
        // for (unsigned int j = 0; j < height; j++) {
        //     const unsigned int indexj = j * width;
        //     for (unsigned int i = 0; i < width; i++) {
        //         unsigned char* const pixel_color = data_buffer[indexj + i];
        //         const real r = pixel_color[0];
        //         const real g = pixel_color[1];
        //         const real b = pixel_color[2];
        //         const unsigned char e = pixel_color[3];
        //         const real radiance_val = pow(2.0f, e - 128);
                
        //         data[i][j] = rt::color(r, g, b) * radiance_val;
        //     }
        // }
        PARALLEL_FOR_BEGIN(width) {
            std::vector<rt::color>& data_line = data[i];
            for (unsigned int j = 0; j < height; j++) {
                unsigned char* const pixel_color = data_buffer[j * width + i];
                const real r = pixel_color[0];
                const real g = pixel_color[1];
                const real b = pixel_color[2];
                const unsigned char e = pixel_color[3];
                const real radiance_val = pow(2.0f, e - 128);
                        
                data_line[j] = rt::color(r * radiance_val, g * radiance_val, b * radiance_val);
            }
        } PARALLEL_FOR_END();

        fclose(file);
        return true;
    }
    catch(const std::exception& e) {
        printf("%s\n", e.what());
        fclose(file);
        return false;
    }
}