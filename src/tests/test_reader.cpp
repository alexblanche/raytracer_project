#include "file_readers/bmp_reader.hpp"
#include "file_readers/hdr_reader.hpp"
#include "screen/screen.hpp"
#include "auxiliary/timer.hpp"

#include <cstdlib>
#include <iostream>


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

    timer_ms timer;
    timer.start();
    for (int k = 0; k < NB_ITERATIONS; k++) {
        read_bmp(filename_bmp, matrix);
    }
    timer.stop();
    printf("BMP ");
    timer.print();

    rt::screen scr(matrix, width, height, tone_mapping_parameters::mode::Disabled);
    scr.fast_copy(1);
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

    timer_ms timer;
    timer.start();
    for (int k = 0; k < NB_ITERATIONS; k++) {
        read_hdr(filename_hdr, matrix);
    }
    timer.stop();
    printf("HDR ");
    timer.print();

    rt::screen scr(matrix, width, height);
    scr.fast_copy(1);
    scr.update_from_texture();
    scr.wait_quit_event();
}

static void test_fastcopy() {
    constexpr const char * filename_bmp = "../../../assets/cobblestone_street_night.bmp";
    

    std::optional<dimensions> dims = read_bmp_size(filename_bmp);
    if (not dims.has_value()) {
        std::cout << "File not found" << std::endl;
        exit(EXIT_FAILURE);
    }

    const auto [ width, height ] = dims.value();
    std::vector<std::vector<rt::color>> matrix(width, std::vector<rt::color>(height));

    read_bmp(filename_bmp, matrix);
    
    rt::screen scr(matrix, width, height);
    // printf("\n");
    timer_ms timer;
    timer.start();
    constexpr int NB_ITERATIONS = 20;
    for (int k = 0; k < NB_ITERATIONS; k++) {
        scr.fast_copy(1);
        scr.update_from_texture();
        if (scr.poll_keyboard_event() == rt::screen::key::QuitEvent)
            return;
        // printf("\r%d / %d         ", (k + 1), NB_ITERATIONS);
        // fflush(stdout);
    }
    timer.stop();
    printf("Fast_copy ");
    timer.print();
}

int main(int argc, char **argv) {

    if (argc < 2) {
        test_fastcopy();
        return EXIT_SUCCESS;
    }

    const std::string arg = argv[1];
    
    if (arg == "bmp")
        test_bmp();
    else if (arg == "hdr")
        test_hdr();
    else
        test_fastcopy();

    return EXIT_SUCCESS;
}