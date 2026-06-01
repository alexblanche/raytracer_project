#include "file_readers/bmp_reader.hpp"
#include "file_readers/hdr_reader.hpp"
#include "screen/screen.hpp"
#include "auxiliary/timer.hpp"

#include <string>
#include <cstdlib>
#include <iostream>
#include <cassert>

static const std::string BMP_FILE_NAME = "../../../assets/cobblestone_street_night.bmp";
static const std::string HDR_FILE_NAME = "../../../assets/sundowner_overlook.hdr";

static void test_bmp() {
    const std::string& filename_bmp = BMP_FILE_NAME;
    constexpr int NB_ITERATIONS = 10;

    std::optional<matrix> mat_opt;

    timer_ms timer;
    timer.start();
    for (int k = 0; k < NB_ITERATIONS; k++) {
        mat_opt = read_bmp(filename_bmp);
        assert(mat_opt.has_value());
    }
    timer.stop();
    printf("BMP ");
    timer.print();

    rt::screen scr(mat_opt.value());
    scr.fast_copy(1);
    scr.update_from_texture();
    scr.wait_quit_event();
}

static void test_hdr() {
    const std::string& filename_hdr = HDR_FILE_NAME;
    constexpr int NB_ITERATIONS = 5;

    std::optional<matrix> mat_opt;

    timer_ms timer;
    timer.start();
    for (int k = 0; k < NB_ITERATIONS; k++) {
        mat_opt = read_hdr(filename_hdr);
        assert(mat_opt.has_value());
    }
    timer.stop();
    printf("HDR ");
    timer.print();

    rt::screen scr(mat_opt.value());
    scr.fast_copy(1);
    scr.update_from_texture();
    scr.wait_quit_event();
}

static void test_fastcopy() {
    const std::string& filename_bmp = BMP_FILE_NAME;
    
    std::optional<matrix> mat_opt = read_bmp(filename_bmp);
    if (not mat_opt.has_value())
        return;
    matrix& matrix = mat_opt.value();
    
    rt::screen scr(matrix);
    timer_ms timer;
    timer.start();
    constexpr int NB_ITERATIONS = 20;
    for (int k = 0; k < NB_ITERATIONS; k++) {
        scr.fast_copy(1);
        scr.update_from_texture();
        if (scr.poll_keyboard_event() == rt::screen::key::QuitEvent)
            return;
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