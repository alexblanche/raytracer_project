#include "file_readers/image_files/bmp_reader.hpp"
#include "file_readers/image_files/hdr_reader.hpp"
#include "file_readers/image_files/raw_data.hpp"
#include "screen/screen.hpp"
#include "auxiliary/timer.hpp"

#include <string>
#include <cstdlib>
#include <iostream>
#include <cassert>

static const std::string BMP_FILE_NAME = "../../../assets/obj/alaskan_cliff_rock/CliffRock_0014_2K_Albedo.bmp";
    //"../../../assets/cobblestone_street_night.bmp";
static const std::string HDR_FILE_NAME = "../../../assets/sundowner_overlook.hdr";

static void test_bmp() {
    const std::string& filename_bmp = BMP_FILE_NAME;
    constexpr int NB_ITERATIONS = 10;

    bmp::print_info(BMP_FILE_NAME);

    std::expected<matrix, file_reader::error> mat_opt;

    timer_ms timer;
    timer.start();
    for (int k = 0; k < NB_ITERATIONS; k++) {
        mat_opt = bmp::read_file(filename_bmp);
        assert(mat_opt.has_value());
    }
    timer.stop();
    printf("BMP ");
    timer.print();

    image img(std::move(mat_opt.value()));
    rt::screen scr(img);
    scr.fast_copy(1);
    scr.update_from_texture();
    scr.wait_quit_event();
}

static void test_write_bmp() {
    const std::string& filename_bmp = BMP_FILE_NAME;
    const std::string output_filename_bmp_base = "../output/TEST/TEST_";
    constexpr int NB_ITERATIONS = 10;

    std::expected<matrix, file_reader::error> mat_opt = bmp::read_file(filename_bmp);
    assert(mat_opt.has_value());
    image img(std::move(mat_opt.value()));
    rt::screen scr(img);
    scr.fast_copy(1);
    scr.update_from_texture();

    timer_ms timer;
    timer.start();
    for (int k = 0; k < NB_ITERATIONS; k++) {
        const std::string output_filename = output_filename_bmp_base + std::to_string(k) + ".bmp";
        const exit_status status = bmp::export_data(output_filename, img);
        assert(status == exit_status::Success);
    }
    timer.stop();
    printf("BMP write ");
    timer.print();

    scr.wait_quit_event();
}

static void test_write_raw() {
    const std::string& filename_bmp = BMP_FILE_NAME;
    const std::string output_filename_raw_base = "../output/TEST/TEST_";
    constexpr int NB_ITERATIONS = 10;

    std::expected<matrix, file_reader::error> mat_opt = bmp::read_file(filename_bmp);
    assert(mat_opt.has_value());
    image img(std::move(mat_opt.value()));
    rt::screen scr(img);
    scr.fast_copy(1);
    scr.update_from_texture();

    timer_ms timer;
    timer.start();
    for (int k = 0; k < NB_ITERATIONS; k++) {
        const std::string output_filename = output_filename_raw_base + std::to_string(k) + ".rtdata";
        const exit_status status = raw_data::export_data(output_filename, img);
        assert(status == exit_status::Success);
    }
    timer.stop();
    printf("Raw data write ");
    timer.print();

    scr.wait_quit_event();
}

static void test_hdr() {
    const std::string& filename_hdr = HDR_FILE_NAME;
    constexpr int NB_ITERATIONS = 10;

    std::expected<matrix, file_reader::error> mat_opt;

    timer_ms timer;
    timer.start();
    for (int k = 0; k < NB_ITERATIONS; k++) {
        mat_opt = hdr::read_file(filename_hdr);
        assert(mat_opt.has_value());
    }
    timer.stop();
    printf("HDR ");
    timer.print();

    image img(std::move(mat_opt.value()));
    rt::screen scr(img);
    scr.fast_copy(1);
    scr.update_from_texture();
    scr.wait_quit_event();
}

static void test_fastcopy() {
    const std::string& filename_bmp = BMP_FILE_NAME;
    
    std::expected<matrix, file_reader::error> mat_opt = bmp::read_file(filename_bmp);
    if (not mat_opt.has_value())
        return;
    
    image img(std::move(mat_opt.value()));
    rt::screen scr(img);
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
    else if (arg == "wbmp")
        test_write_bmp();
    else if (arg == "wraw")
        test_write_raw();
    else
        test_fastcopy();

    return EXIT_SUCCESS;
}