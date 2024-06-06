#include <iostream>

#include "file_readers/bmp_reader.hpp"
#include "screen/screen.hpp"

#include <algorithm>
#include <thread>
#include <mutex>
#include <functional>
#include <chrono>

#include<unistd.h>

/* Attempt at a real-time skydome */

void update_screen(rt::screen& scr, rt::color& col) {
    scr.clear();
    scr.fill_rect(0, 0, scr.width()-1, scr.height()-1, col);
    // scr.update();
}

uint64_t get_time() {
    return duration_cast< std::chrono::milliseconds >(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

int main(int, char**) {

    /* Skydome texture */
    /*
    const char* file_name = "../../../raytracer_project/sky/dome/field.bmp";

    std::optional<dimensions> dims = read_bmp_size(file_name);
    if (not dims.has_value()) {
        std::cout << "File not found" << std::endl;
        return EXIT_FAILURE;
    }
    std::vector<std::vector<rt::color>> matrix(dims.value().width, std::vector<rt::color>(dims.value().height));
    const bool read_success = read_bmp(file_name, matrix);
    */

    /* Screen dimensions */
    int width = 1366, height = 768;

    rt::screen scr(width, height);

    rt::color col(0, 0, 0);

    bool stop = false;

    std::function<void(void)> func =
        [&] () {

        for(int i = 0; i < 256; i++) {
            col.set_red(i);
            update_screen(scr, col);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        std::mutex m;
        m.lock();
        stop = true;
        m.unlock();
    };

    std::thread th(func);
    
    // func();

    const uint64_t time_init = get_time();
    uint64_t last_update_time = time_init;

    unsigned int cpt = 0;
    
    while (not stop) {
        const uint64_t time = get_time();
        if (time - last_update_time >= 16) {
            last_update_time = time;
            scr.update();
            cpt ++;
        }
    }

    const uint64_t curr_time = get_time();

    std::cout << "Average fps: " << (1000.0 * cpt) / (curr_time - time_init) << std::endl;

    th.join();

    return EXIT_SUCCESS;
}