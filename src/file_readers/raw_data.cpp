#include "file_readers/raw_data.hpp"
#include "file_readers/bmp_reader.hpp"
#include <stdio.h>

/* Creates a file file_name, and writes in it the resolution of the image, the number of rays used to generate it,
   and the raw data of the matrix
   Returns true if the operation was successful */
/* Format:

   width:1366 height:768 number_of_rays:10
   2550.0000000000 2000.0000000000 1367.4666666666
   ...
   
   width * height of 3 doubles (red, green, blue) separated by spaces,
   pixel (i,j) at line j*width + i
   line k representing pixel (k % width, k / width)
*/
bool export_raw(const char* file_name, const unsigned int number_of_rays, std::vector<std::vector<rt::color>>& matrix) {

    FILE* file = fopen(file_name, "w");

    if (file == NULL) {
        printf("Error, file %s could not be created\n", file_name);
        return false;
    }
    
    const unsigned int width = matrix.size();
    const unsigned int height = matrix.at(0).size();

    const int ret0 = fprintf(file, "width:%u height:%u number_of_rays:%u\n",
        width, height, number_of_rays);

    if (ret0 < 0) {
        printf("Writing error at first line of %s\n", file_name);
        fclose(file);
        return false;
    }

    for(unsigned int j = 0; j < height; j++) {
        for(unsigned int i = 0; i < width; i++) {
            rt::color c = matrix.at(i).at(j);
            const int ret = fprintf(file, "%lf %lf %lf\n", c.get_red(), c.get_green(), c.get_blue());
            if (ret < 0) {
                printf("Writing error at color line %u of %s\n", width*j + i, file_name);
                fclose(file);
                return false;
            }
        }
    }

    fclose(file);
    return true;
}

/* Reads a file file_name generated by export_raw, and returns a matrix with its content
   Returns true if the operation was successful */
std::vector<std::vector<rt::color>>& read_raw(const char* file_name, bool& success) {

    FILE* file = fopen(file_name, "r");

    if (file == NULL) {
        printf("Error, file %s not found\n", file_name);
        success = false;
        std::vector<std::vector<rt::color>>* fail_matrix = new std::vector<std::vector<rt::color>>(1, std::vector<rt::color>(1));
        return *fail_matrix;
    }

    unsigned int width, height, number_of_rays;
    const int ret0 = fscanf(file, "width:%u height:%u number_of_rays:%u\n",
        &width, &height, &number_of_rays);

    if (ret0 < 0) {
        printf("Reading error at first line of file %s\n", file_name);
        success = false;
        fclose(file);
        std::vector<std::vector<rt::color>>* fail_matrix = new std::vector<std::vector<rt::color>>(1, std::vector<rt::color>(1));
        return *fail_matrix;
    }

    std::vector<std::vector<rt::color>>* matrix = new std::vector<std::vector<rt::color>>(width, std::vector<rt::color>(height));

    for(unsigned int j = 0; j < height; j++) {
        for(unsigned int i = 0; i < width; i++) {
            double r, g, b;
            const int ret = fscanf(file, "%lf %lf %lf\n", &r, &g, &b);
            if (ret < 0) {
                printf("Reading error at color line %u of file %s\n", width*j + i, file_name);
                fclose(file);
                success = false;
                return *matrix;
            }
            (*matrix).at(i).at(j) = rt::color(r, g, b);
        }
    }

    fclose(file);

    success = true;
    return *matrix;
}

/* Combines the n files whose names are in the array source_file_names into one bmp file dest_file_name
   Returns true if the operation was successful */
bool combine_raw(const char* dest_file_name, const int n, const char* const source_file_names[]) {
    if (n < 0) {
        printf("Error, not enough files provided\n");
        return false;
    }

    FILE* file0 = fopen(source_file_names[0], "r");

    if (file0 == NULL) {
        printf("Error, first file (%s) not found\n", source_file_names[0]);
        return false;
    }

    /* Determination of the size of the matrix */
    unsigned int width, height;
    const int ret0 = fscanf(file0, "width:%u height:%u",
        &width, &height);

    if (ret0 < 0) {
        printf("Reading error at first line of file %s\n", source_file_names[0]);
        fclose(file0);
        return false;
    }

    std::vector<std::vector<rt::color>> matrix(width, std::vector<rt::color>(height));

    unsigned int total_number_of_rays = 0;

    /* Adding the value of each file to matrix */
    for (unsigned int k = 0; k < (unsigned int) n; k++) {
        FILE* file = fopen(source_file_names[k], "r");
        if (file == NULL) {
            printf("Error, file (%s) not found\n", source_file_names[k]);
            return false;
        }

        unsigned int width_k, height_k, number_of_rays_k;
        const int retk0 = fscanf(file, "width:%u height:%u number_of_rays:%u\n",
            &width_k, &height_k, &number_of_rays_k);

        if (retk0 < 0) {
            printf("Reading error at first line of file %s\n", source_file_names[k]);
            fclose(file);
            return false;
        }

        if (width_k != width || height_k != height) {
            printf("Error, incorrect width or height in file %s\n", source_file_names[k]);
            fclose(file);
            return false;
        }

        total_number_of_rays += number_of_rays_k;

        for(unsigned int j = 0; j < height; j++) {
            for(unsigned int i = 0; i < width; i++) {
                double r, g, b;
                const int ret = fscanf(file, "%lf %lf %lf\n", &r, &g, &b);
                if (ret < 0) {
                    printf("Reading error at color line %u of file %s\n", width*j + i, source_file_names[k]);
                    fclose(file);
                    return false;
                }
                matrix.at(i).at(j) = matrix.at(i).at(j) + rt::color(r, g, b);
            }    
        }
        fclose(file);
    }

    /* Exporting the matrix as a bmp file */
    const bool success = write_bmp(dest_file_name, matrix, total_number_of_rays);

    if (success) {
        return true;
    }
    else {
        printf("Error in generating the bmp file %s\n", dest_file_name);
        return false;
    }
}