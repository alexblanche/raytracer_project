#include "file_readers/bmp_reader.hpp"
#include "file_readers/hdr_reader.hpp"
#include "screen/screen.hpp"

#include <cstdlib>
#include <iostream>
#include <chrono>

/* Returns the current time in milliseconds */
uint64_t get_time() {
    return duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

static void test_bmp() {
    constexpr const char * filename_bmp = "../../../assets/cobblestone_street_night.bmp";
    constexpr int NB_ITERATIONS = 10;

    std::optional<dimensions> dims = read_bmp_size(filename_bmp);
    if (not dims.has_value()) {
        std::cout << "File not found" << std::endl;
        exit(EXIT_FAILURE);
    }

    const auto [ width, height ] = dims.value();
    std::vector<std::vector<rt::color>> matrix(width, std::vector<rt::color>(height));

    const uint64_t time_start = get_time();
    for (int k = 0; k < NB_ITERATIONS; k++) {
        read_bmp(filename_bmp, matrix);
    }
    const uint64_t time_end = get_time();
    printf("BMP: Time: %llu ms\n", time_end - time_start);

    rt::screen scr(width, height);
    scr.fast_copy(matrix, width, height, 1);
    scr.update_from_texture();
    scr.wait_quit_event();
}

static void test_hdr() {
    constexpr const char * filename_hdr = "../../../assets/sundowner_overlook.hdr";
    constexpr int NB_ITERATIONS = 5;

    std::optional<dimensions> dims = read_hdr_size(filename_hdr);
    if (not dims.has_value()) {
        std::cout << "File not found" << std::endl;
        exit(EXIT_FAILURE);
    }

    const auto [ width, height ] = dims.value();
    std::vector<std::vector<rt::color>> matrix(width, std::vector<rt::color>(height));

    const uint64_t time_start = get_time();
    for (int k = 0; k < NB_ITERATIONS; k++) {
        read_hdr(filename_hdr, matrix);
    }
    const uint64_t time_end = get_time();
    printf("HDR: Time: %llu ms\n", time_end - time_start);

    rt::screen scr(width, height);
    scr.fast_copy(matrix, width, height, 1);
    scr.update_from_texture();
    scr.wait_quit_event();
}

int main() {

    constexpr bool b = true;
    if (b)
        test_bmp();
    else
        test_hdr();

    return EXIT_SUCCESS;
}