#include <iostream>
#include <cstdlib>
#include "screen/screen.hpp"

#include "file_readers/raw_data.hpp"
#include "file_readers/bmp_reader.hpp"
#include "file_readers/scene_parser.hpp"

#include "render/render_loops.hpp"

#include "main_menu/runtime_parameters.hpp"
#include "main_menu/file_handler.hpp"
#include "main_menu/menu.hpp"

#include <filesystem>

/** Loop keys:
 * Space/Enter: Continue
 * B key:       Save as image.bmp
 * R key:       Save raw data as image.rtdata
 * Esc:         Exit
 */

int main(int argc, char *argv[]) {

    menu menu;
    const exit_status status_parse = menu.parse_arguments(argc, argv);
    if (status_parse == exit_status::Failure)
        return EXIT_FAILURE;
    
    const std::optional<scene> scene_opt = menu.parse_scene_descriptor_file();
    if (not scene_opt.has_value()) {
        printf("Scene creation failed\n");
        return EXIT_FAILURE;
    }

    const scene& scene = scene_opt.value();
    menu.update_gamma(scene.gamma);
    printf("Number of objects: %zu\n", scene.object_set.size());
    
    const exit_status status_run = menu.run(scene);
    if (status_run == exit_status::Failure)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}