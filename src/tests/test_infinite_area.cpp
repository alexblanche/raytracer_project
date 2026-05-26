#include "file_readers/hdr_reader.hpp"
#include "scene/light_sources/infinite_area.hpp"
#include "screen/screen.hpp"

// printf("real : %llu, color : %llu, vector : %llu\n", sizeof(real), sizeof(rt::color), sizeof(rt::vector));
// printf("real : %llu\n", sizeof(real));
// printf("size_t : %llu ; unsigned int : %llu\n", sizeof(size_t), sizeof(unsigned int));
// printf("unsigned short : %llu ; unsigned char : %llu\n", sizeof(unsigned short int), sizeof(unsigned char));
// printf("material : %llu with alignment %llu\n", sizeof(material), alignof(material));
// printf("object : %llu with alignment %llu\n", sizeof(object), alignof(object));
// printf("texture_info : %llu with alignment %llu\n", sizeof(texture_info), alignof(texture_info));
// printf("triangle : %llu with alignment %llu\n", sizeof(triangle), alignof(triangle));
// printf("sphere : %llu with alignment %llu\n", sizeof(sphere), alignof(sphere));
// printf("vector : %llu with alignment %llu\n", sizeof(rt::vector), alignof(rt::vector));
// printf("plane : %llu with alignment %llu\n", sizeof(plane), alignof(plane));
// printf("\n");

///// Testing low res infinite area
int main() {

    const char* file_name = //"../../../raytracer_project/sky/dome/garden_8k.hdr";
        "../../../assets/sundowner_overlook.hdr";

    std::optional<dimensions> size = read_hdr_size(file_name);
    if (not size.has_value())
        return EXIT_FAILURE;

    const auto [ width, height ] = size.value();
    std::vector<std::vector<rt::color>> data(width, std::vector<rt::color>(height));

    const exit_status hdr_success = read_hdr(file_name, data);
    if (hdr_success == exit_status::Failure)
        return EXIT_FAILURE;

    const std::vector<real> lrt = alias_table::compute_low_res_table(data);
    const unsigned int dwidth  = LOWRES_DEFAULT_WIDTH;
    const unsigned int dheight = LOWRES_DEFAULT_HEIGHT;
    std::vector<std::vector<rt::color>> lrdata(dwidth, std::vector<rt::color>(dheight));
    // for (unsigned int i = 0; i < LOWRES_DEFAULT_WIDTH; i++) {
    //     for (unsigned int j = 0; j < LOWRES_DEFAULT_HEIGHT; j++) {
    //         const real x = 255.0f * 100000.0f * lrt[j * LOWRES_DEFAULT_WIDTH + i];
    //         //printf("%f\n", x);
    //         lrdata[i][j] = rt::color(x, x, x);
    //     }
    // }

    // const rt::screen test_scr(LOWRES_DEFAULT_WIDTH, LOWRES_DEFAULT_HEIGHT);
    // test_scr.fast_copy(lrdata, LOWRES_DEFAULT_WIDTH, LOWRES_DEFAULT_HEIGHT, 1);
    // test_scr.update_from_texture();
    // test_scr.wait_keyboard_event();

    alias_table alt(lrt, width, height, dwidth, dheight);
    const randomgen rand;
    // std::vector<unsigned int> samples(1000);
    // for (unsigned int i = 0; i < samples.size(); i++) {
    //     samples[i] = alt.sample(rand);
    // }
    // std::sort (samples.begin(), samples.end());

    // for (unsigned int i = 0; i < samples.size(); i++) {
    //     printf("%u %u\n", i, samples[i]);
    // }

    const rt::screen test_scr(lrdata, dwidth, dheight);
    constexpr rt::color color_one(1.0f, 1.0f, 1.0f);

    while (true) {
        for (int i = 0; i < 1000000; i++) {
            const unsigned int s = alt.sample(rand);
            rt::color& px = lrdata[s % dwidth][s / dheight];
            px += color_one;
        }
        test_scr.fast_copy(1);
        test_scr.update_from_texture();
        if (test_scr.poll_keyboard_event() == rt::screen::key::QuitEvent)
            break;
    }

    ////
    // Works perfectly!
    ////

    return EXIT_SUCCESS;
}