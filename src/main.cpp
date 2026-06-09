#include "main_menu/menu.hpp"

#include <iostream>

/** Loop keys:
 * Space/Enter: Continue
 * B key:       Save as image.bmp
 * R key:       Save raw data as image.rtdata
 * Esc:         Exit
 */

int main(int argc, char *argv[]) {

    // {
    //     double x = 702.806753;
    //     double y = 982.626482;
    //     double z = 1539.842054;
    //     printf("%lf %lf %lf\n", x, y, z);
    //     file f("test.txt", "w");
    //     double t[3] = { x, y, z };
    //     for (int i = 0; i < 100; i++) {
    //         f.write<double>({t, 3});
    //     }
    //     f.close();

    //     file lf("test_long.txt", "w");
    //     for (int i = 0; i < 100; i++) {
    //         lf.printf("%lf %lf %lf\n", x, y, z);
    //     }
    // }

    // {
    //     file f("test.txt", "r");
    //     printf("length = %zu\n", f.length());
    //     double t[3];
    //     const exit_status status = f.read<double, 3>(t);
    //     if (status == exit_status::Failure)
    //         return 1;
    //     printf("%lf %lf %lf\n", t[0], t[1], t[2]);

    //     f.close();

    //     file lf("test_long.txt", "r");
    //     printf("long: length = %zu\n", lf.length());
    // }
    // return 0;

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