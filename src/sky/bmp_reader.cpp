#include "sky/bmp_reader.hpp"

#include <cstdio>
#include <stdexcept>
#include <cmath>

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
exit_status read_bmp(const char* file_name, std::vector<std::vector<sky::color>>& data) {
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
                data[i][indexj] = sky::color(r, g, b);
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

/* Prints the info contained in the header of the given .bmp file */
exit_status print_bmp_info(const char* file_name) {
    FILE* file = fopen(file_name, "rb");

    if (file == nullptr) {
        printf("Error, file %s not found\n", file_name);
        return exit_status::Failure;
    }

    char filetype[3];
    const int ret1 = fscanf(file, "%2s", filetype);
    if (ret1 != 1) {
        fclose(file);
        printf("Reading error in read_bmp type\n");
        return exit_status::Failure;
    }
    printf("Type:                  %2s\n", filetype);

    bmp_header header;
    const int ret2 = fread(static_cast<void*>(&header), sizeof(bmp_header), 1, file);
    fclose(file);

    if (ret2 != 1) {
        printf("Reading error in read_bmp header\n");
        return exit_status::Failure;
    }
    
    header.print();
    return exit_status::Success;
}
