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

/* Returns the current time in milliseconds */
uint64_t get_time() {
    return duration_cast< std::chrono::milliseconds >(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

struct mouse_pos {
    int x;
    int y;

    void set(int mx, int my) {
        x = mx;
        y = my;
    }
};

void update_screen(rt::screen& scr, const rt::color& bg_col, const rt::color& cursor_col, mouse_pos& mouse) {
    scr.clear();
    scr.fill_rect(0, 0, scr.width()-1, scr.height()-1, bg_col);
    //SDL_GetMouseState(&mouse.x, &mouse.y);
    scr.draw_line(
        mouse.x - 8, mouse.y,
        mouse.x + 8, mouse.y,
        cursor_col
    );
    scr.draw_line(
        mouse.x, mouse.y - 8,
        mouse.x, mouse.y + 8,
        cursor_col
    );
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
    //int width = 1366, height = 768;
    int width = 1920, height = 1080;

    rt::screen scr(width, height);
    

    mouse_pos mouse;
    mouse.set(scr.width()/2, scr.height()/2);

    const rt::color bg_col(0, 0, 0);
    const rt::color cursor_col(255, 0, 0);
    update_screen(scr, bg_col, cursor_col, mouse);

    bool stop = false;

    /*
    // Fade from black to red
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
    */

    
    std::function<void(void)> func =
        [&] () {
        
        SDL_Event event;
        while(SDL_WaitEvent(&event)) {
            /*
            switch(event.type) {
                case SDL_MOUSEMOTION:
                    mouse.set(event.motion.x, event.motion.y);
                    break;
                case SDL_QUIT:
                case SDL_KEYDOWN:
                    if (event.type == SDL_QUIT ||
                        (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)) {
                        stop = true;
                        return;
                    }
                    break;
                default:
                    break;
            }
            */
                
            if (event.type == SDL_MOUSEMOTION) {
                mouse.set(event.motion.x, event.motion.y);
            }
            else if (event.type == SDL_QUIT ||
                (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)) {
                stop = true;
                return;
            }
        }
    };

    std::thread th(func);
    

    const uint64_t time_init = get_time();
    uint64_t last_update_time = time_init;

    unsigned int frame_cpt = 0;
    
    /* Update loop */

    while (not stop) {
        const uint64_t time = get_time();

        /*
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
                
            if (event.type == SDL_MOUSEMOTION) {
                mouse.set(event.motion.x, event.motion.y);
            }
            else if (event.type == SDL_QUIT ||
                (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)) {
                stop = true;
                break;
            }
        }
        */

        /* Update every 16ms, to target 60fps */
        if (time - last_update_time >= 1000 / 60) {
            update_screen(scr, bg_col, cursor_col, mouse);
            scr.update();
            last_update_time = time;
            frame_cpt ++;
        }
    }

    const uint64_t curr_time = get_time();

    std::cout << "Average fps: " << (1000.0 * frame_cpt) / (curr_time - time_init) << std::endl;

    th.join();

    return EXIT_SUCCESS;
}