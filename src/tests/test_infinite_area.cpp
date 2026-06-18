#include "file_readers/image_files/hdr_reader.hpp"
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

    const std::string file_name =
        //  "../../../raytracer_project/sky/dome/garden_8k.hdr";
            "../../../assets/sundowner_overlook.hdr";

    const std::optional<matrix> mat_opt = hdr::read_file(file_name);
    if (not mat_opt.has_value())
        return EXIT_FAILURE;

    constexpr unsigned int dwidth  = LOWRES_DEFAULT_WIDTH;
    constexpr unsigned int dheight = LOWRES_DEFAULT_HEIGHT;
    image lrdata(dwidth, dheight);

    const alias_table alt(mat_opt.value(), dwidth, dheight);
    const random_ratio_gen<alias_table::Float> rg = alt.get_random_generator();

    const rt::screen test_scr(lrdata);
    constexpr rt::color color_one(1, 1, 1);

    constexpr int batch_size = 10 * 100000;
    timer_ms timer;
    int cpt = 0;
    while (true) {
        timer.start();
        for (int i = 0; i < batch_size; i++) {
            const unsigned int s = alt.sample_table(rg);
            lrdata[s / dwidth, s % dwidth] += color_one;
        }
        test_scr.fast_copy(1);
        test_scr.update_from_texture();
        if (test_scr.poll_keyboard_event() == rt::screen::key::QuitEvent)
            break;
        timer.stop();
        const uint64_t elapsed = timer.elapsed();
        cpt++;
        if ((cpt & 3) == 0) {
            printf("\r%.2f fps", 1000.0f / elapsed);
            fflush(stdout);
        }
    }

    return EXIT_SUCCESS;
}