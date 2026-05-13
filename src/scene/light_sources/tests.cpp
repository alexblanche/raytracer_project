// #include "file_readers/hdr_reader.hpp"
// #include "scene/light_sources/infinite_area.hpp"

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

///// TEMP: testing low res infinite area
/*
const char* file_name = "../../../raytracer_project/sky/dome/garden_8k.hdr";
std::optional<dimensions> size = read_hdr_size(file_name);
std::vector<std::vector<rt::color>> data(size.value().width, std::vector<rt::color>(size.value().height));
bool hdr_success = read_hdr(file_name, data);
if (!hdr_success) throw;

std::vector<real> lrt = compute_low_res_table(data);
std::vector<std::vector<rt::color>> lrdata(854, std::vector<rt::color>(480));
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

alias_table alt(lrt, size.value().width, size.value().height, LOWRES_DEFAULT_WIDTH, LOWRES_DEFAULT_HEIGHT);
randomgen rand(ANTI_ALIASING);
// std::vector<unsigned int> samples(1000);
// for (unsigned int i = 0; i < samples.size(); i++) {
//     samples[i] = alt.sample(rand);
// }
// std::sort (samples.begin(), samples.end());

// for (unsigned int i = 0; i < samples.size(); i++) {
//     printf("%u %u\n", i, samples[i]);
// }

const rt::screen test_scr(LOWRES_DEFAULT_WIDTH, LOWRES_DEFAULT_HEIGHT);
const rt::color color_one(1.0f, 1.0f, 1.0f);

while (true) {
    for (unsigned int i = 0; i < 1000000; i++) {
        unsigned int s = alt.sample(rand);
        rt::color& px = lrdata[s % LOWRES_DEFAULT_WIDTH][s / LOWRES_DEFAULT_WIDTH];
        px = px + color_one;
    }
    test_scr.fast_copy(lrdata, LOWRES_DEFAULT_WIDTH, LOWRES_DEFAULT_HEIGHT, 1);
    test_scr.update_from_texture();
}

// Works perfectly!
*/
////