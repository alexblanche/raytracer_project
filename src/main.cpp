#include "main_menu/menu.hpp"

#include <iostream>

/** Loop keys:
 * Space/Enter: Continue
 * B key:       Save as image.bmp
 * R key:       Save raw data as image.rtdata
 * Esc:         Exit
 */

int main(int argc, char *argv[]) {


    for (double x = 1.0e-10; x < 3; x += 0.05) {
        printf("x = %lf: ", x);
        uint64_t n = *reinterpret_cast<uint64_t*>(&x);
        constexpr int N = 12;
        n >>= (64 - N);
        for (int i = 0; i < N; i++) {
            printf("%llu", n & 1);
            n >>= 1;
        }
        printf("\n");
    }


    const std::vector<std::string> args(argv + 1, argv + argc);

    menu menu;
    exit_if_failure(menu.parse_arguments(args));
    
    const std::optional<scene> scene_opt = menu.parse_scene_descriptor_file();
    if (not scene_opt.has_value())
        return EXIT_FAILURE;

    const scene& scene = scene_opt.value();
    menu.update_gamma(scene.gamma);
    printf("Number of objects: %zu\n", scene.object_set.size());
    
    exit_if_failure(menu.run(scene));

    return EXIT_SUCCESS;
}