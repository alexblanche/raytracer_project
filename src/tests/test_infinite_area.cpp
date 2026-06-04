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

    const std::optional<matrix> mat_opt = read_hdr(file_name);
    if (not mat_opt.has_value())
        return EXIT_FAILURE;

    const matrix& mat = mat_opt.value();
    const std::vector<real> lrt = alias_table::compute_low_res_table(mat.data);
    const unsigned int dwidth  = LOWRES_DEFAULT_WIDTH;
    const unsigned int dheight = LOWRES_DEFAULT_HEIGHT;
    matrix lrdata(dwidth, dheight);

    alias_table alt(lrt, mat.width, mat.height, dwidth, dheight);
    const randomgen rand;
    // std::vector<unsigned int> samples(1000);
    // for (unsigned int i = 0; i < samples.size(); i++) {
    //     samples[i] = alt.sample(rand);
    // }
    // std::sort (samples.begin(), samples.end());

    // for (unsigned int i = 0; i < samples.size(); i++) {
    //     printf("%u %u\n", i, samples[i]);
    // }

    const rt::screen test_scr(lrdata);
    constexpr rt::color color_one(1, 1, 1);

    while (true) {
        for (int i = 0; i < 1000000; i++) {
            const unsigned int s = alt.sample(rand);
            rt::color& px = lrdata.data[s % dwidth][s / dheight];
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