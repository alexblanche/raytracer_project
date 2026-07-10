#include "main_menu/menu.hpp"

#include <SDL2/SDL.h>
#include <iostream>

/** Loop keys:
 * Space/Enter: Continue
 * B key:       Save as image.bmp
 * R key:       Save raw data as image.rtdata
 * Esc:         Exit
 */

int main(int argc, char *argv[]) {
    
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