#include "file_readers/image_files/bmp_reader.hpp"
#include "file_readers/image_files/hdr_reader.hpp"
#include "file_readers/image_files/raw_data.hpp"
#include "file_readers/parsers/obj_parser.hpp"
#include "file_readers/parsers/scene_parser.hpp"

#include "screen/screen.hpp"
#include "auxiliary/timer.hpp"

#include <string>
#include <cstdlib>
#include <iostream>
#include <cassert>

static const std::string BMP_FILE_NAME = "../../../assets/cobblestone_street_night.bmp";
static const std::string HDR_FILE_NAME = "../../../assets/sundowner_overlook.hdr";
static const std::string OBJ_FILE_NAME = "../../../assets/obj/alaskan_cliff_rock/CliffRock_0014_High.obj";

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

static void test_obj() {
    const std::string& filename_obj = OBJ_FILE_NAME;
    constexpr int NB_ITERATIONS = 5;

    pre_parsing_info pre_parsing_info;
    const auto& [ _, obj_triangles, obj_quads ] = pre_parse_obj(filename_obj);
    pre_parsing_info.triangles += obj_triangles;
    pre_parsing_info.quads     += obj_quads;
    pre_parsing_info.objects   += obj_triangles + obj_quads;

    std::vector<const object*>       object_set;
    object_set.reserve(pre_parsing_info.objects);

    std::vector<const object*>       other_content;
    other_content.reserve(
        pre_parsing_info.spheres + pre_parsing_info.planes + pre_parsing_info.boxes + pre_parsing_info.cylinders
    );

    std::vector<wrapper<material>>   material_wrapper_set;
    std::vector<wrapper<texture>>    texture_wrapper_set;
    std::vector<wrapper<normal_map>> normal_map_wrapper_set;
    std::vector<texture_info>        texture_info_set;

    std::vector<triangle> triangle_set;
    std::vector<quad>     quad_set;
    std::vector<sphere>   sphere_set;
    std::vector<plane>    plane_set;
    std::vector<box>      box_set;
    std::vector<cylinder> cylinder_set;

    triangle_set.reserve(pre_parsing_info.triangles + 2 * pre_parsing_info.quads);
    quad_set    .reserve(pre_parsing_info.quads);
    sphere_set  .reserve(pre_parsing_info.spheres);
    plane_set   .reserve(pre_parsing_info.planes);
    box_set     .reserve(pre_parsing_info.boxes);
    cylinder_set.reserve(pre_parsing_info.cylinders);

    containers containers = {
        object_set,
        other_content,

        triangle_set,
        quad_set,
        sphere_set,
        plane_set,
        box_set,
        cylinder_set,

        material_wrapper_set,
        texture_wrapper_set,
        normal_map_wrapper_set,
        texture_info_set
    };

    uint64_t total_time = 0;

    timer_ms timer;
    for (int k = 0; k < NB_ITERATIONS; k++) {

        timer.start();
        
        const bounding* output_bd = nullptr;
        const exit_status status = parse_obj_file(filename_obj, std::nullopt, containers, 2.0_r, rt::vector(1, 1, 1), false, 0, output_bd, 1.0_r);
        assert(status == exit_status::Success);

        timer.stop();
        total_time += timer.elapsed();

        ////
        
        //timer.start();
        for (auto pt : object_set)
            delete pt;
        //timer.stop();
        //timer.print();
        object_set.clear();
        material_wrapper_set.clear();
        texture_wrapper_set.clear();
        normal_map_wrapper_set.clear();
        texture_info_set.clear();
    }
    printf("OBJ ");
    printf("Time: %llums\n", total_time);
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
    else if (arg == "obj")
        test_obj();
    else
        test_fastcopy();

    return EXIT_SUCCESS;
}