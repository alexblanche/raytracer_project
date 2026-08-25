#include "file_readers/image_files/bmp_reader.hpp"
#include "file_readers/image_files/hdr_reader.hpp"
#include "file_readers/image_files/raw_data.hpp"
#include "file_readers/parsers/obj_parser.hpp"
#include "file_readers/parsers/scene_parser.hpp"

#include "screen/screen.hpp"
#include "auxiliary/timer.hpp"

#include <string>
#include <cstdlib>
#include <cassert>
#include <optional>

static const std::string BMP_FILE_NAME = "../../../assets/sky/cobblestone_street_night.bmp";
static const std::string HDR_FILE_NAME = "../../../assets/sky/sundowner_overlook.hdr";
static const std::string OBJ_FILE_NAME = "../../../assets/obj/alaskan_cliff_rock/CliffRock_0014_High.obj";

static void test_bmp(std::optional<std::string> arg = std::nullopt) {
    const std::string& filename_bmp = arg.value_or(BMP_FILE_NAME);
    constexpr int NB_ITERATIONS = 10;

    bmp::print_info(filename_bmp);

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

static void test_write_bmp(std::optional<std::string> arg = std::nullopt) {
    const std::string& filename_bmp = arg.value_or(BMP_FILE_NAME);
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
        [[maybe_unused]] const exit_status status = bmp::export_data(output_filename, img);
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
        [[maybe_unused]] const exit_status status = raw_data::export_data(output_filename, img);
        assert(status == exit_status::Success);
    }
    timer.stop();
    printf("Raw data write ");
    timer.print();

    scr.wait_quit_event();
}

static void test_hdr(std::optional<std::string> arg = std::nullopt) {
    const std::string& filename_hdr = arg.value_or(HDR_FILE_NAME);
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

    constexpr double gamma = 1.0 / 2.2;
    image img(std::move(mat_opt.value()), gamma);
    rt::screen scr(img);
    scr.fast_copy_gamma(1);
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
    runtime_debugger debug {};
    timer.start();
    constexpr int NB_ITERATIONS = 20;
    for (int k = 0; k < NB_ITERATIONS; k++) {
        scr.fast_copy(1);
        scr.update_from_texture();
        if (scr.poll_keyboard_event(debug) == rt::screen::key::QuitEvent)
            return;
    }
    timer.stop();
    printf("Fast_copy ");
    timer.print();
}

static void test_obj() {
    const std::string& filename_obj = OBJ_FILE_NAME;
    constexpr int NB_ITERATIONS = 5;

    scene::pre_parsing_info pre_parsing_info;
    const auto& [ _, obj_triangles, obj_quads ] = pre_parse_obj(filename_obj);
    pre_parsing_info.triangles += obj_triangles;
    pre_parsing_info.quads     += obj_quads;
    pre_parsing_info.objects   += obj_triangles + obj_quads;

    std::vector<const object*> object_set;
    object_set.reserve(pre_parsing_info.objects);

    std::vector<const object*> other_content;
    other_content.reserve(
          pre_parsing_info.spheres
        + pre_parsing_info.planes
        + pre_parsing_info.boxes
        + pre_parsing_info.cylinders
    );

    std::vector<wrapper<material>>    material_wrapper_set;
    std::vector<wrapper<composition>> composition_wrapper_set;
    std::vector<texture>    texture_set;
    std::vector<normal_map> normal_map_set;

    scene::containers::object object_containers(pre_parsing_info);
    scene::containers::orientation orientation_containers(pre_parsing_info);
    

    containers containers = {
        object_set,
        other_content,
        object_containers,
        material_wrapper_set,
        composition_wrapper_set,
        texture_set,
        normal_map_set,
        orientation_containers
    };

    uint64_t total_time = 0;

    timer_ms timer;
    for (int k = 0; k < NB_ITERATIONS; k++) {

        timer.start();
        
        std::optional<bvh> empty_bvh;
        [[maybe_unused]] const exit_status status = parse_obj_file(
            filename_obj, std::nullopt, containers,
            model_positioning(rt::vector(1, 1, 1), 2.0_r),
            empty_bvh, 1.0_r
        );
        assert(status == exit_status::Success);

        timer.stop();
        total_time += timer.elapsed();

        ////
        
        //timer.start();
        
        auto& [ triangle_set, quad_set, _, _, _, _ ] = object_containers;
        auto& [ triangle_orientation_set, quad_orientation_set, _, _, _, _ ] = orientation_containers;
        triangle_set.clear();
        triangle_orientation_set.clear();
        quad_set.clear();
        quad_orientation_set.clear();
        composition_wrapper_set.clear();

        //timer.stop();
        //timer.print();
        object_set.clear();
        material_wrapper_set.clear();
        texture_set.clear();
        normal_map_set.clear();
    }
    printf("OBJ ");
    printf("Time: %lums\n", static_cast<unsigned long int>(total_time));
}

static void convert_hdr_to_bmp(const std::string& filename_hdr) {
    std::expected<matrix, file_reader::error> mat_opt = hdr::read_file(filename_hdr);
    assert(mat_opt.has_value());
    constexpr double gamma = 1.0 / 2.2;
    image img(std::move(mat_opt.value()), gamma);
    const std::string output_filename = filename_hdr.substr(0, filename_hdr.length() - 4) + ".bmp";
    const exit_status status = bmp::export_data(output_filename, img);
    throw_if_failure(status, "Error hdr to bmp conversion");
    printf("File %s created\n", output_filename.c_str());
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        test_fastcopy();
        return EXIT_SUCCESS;
    }

    const std::string cmd = argv[1];

    const bool arg_provided = argc >= 3;
    std::string arg;
    if (arg_provided)
        arg = argv[2];

    const std::optional<std::string> arg_opt = (arg_provided) ? std::optional(arg) : std::nullopt;

    if (cmd == "bmp")
        test_bmp(arg_opt);
    else if (cmd == "hdr")
        test_hdr(arg_opt);
    else if (cmd == "wbmp")
        test_write_bmp(arg_opt);
    else if (cmd == "wraw")
        test_write_raw();
    else if (cmd == "obj")
        test_obj();
    else if (cmd == "conv" && arg_provided)
        convert_hdr_to_bmp(arg);
    else
        test_fastcopy();

    return EXIT_SUCCESS;
}