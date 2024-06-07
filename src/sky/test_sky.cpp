#include <iostream>

#include "file_readers/bmp_reader.hpp"
#include "screen/screen.hpp"

#include <algorithm>
// #include <thread>
// #include <mutex>
// #include <functional>
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

void update_screen_cursor(rt::screen& scr, const rt::color& bg_col, const rt::color& cursor_col, mouse_pos& mouse) {
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

void update_screen(rt::screen& scr, std::vector<std::vector<rt::color>>& image, mouse_pos& /*mouse*/) {
    scr.clear();
    const float ratio_x = ((float) image.size()) / (scr.width()-1);
    const float ratio_y = ((float) image[0].size()) / (scr.height()-1);
    for (int i = 0; i < scr.width()-1; i++) {
        for (int j = 0; j < scr.height()-1; j++) {
            scr.set_pixel(i, j, image[(int) (ratio_x * i)][(int) (ratio_y * j)]);
        }
    }
}

int main(int, char**) {

    /* Skydome texture */
    const char* file_name = "../../../raytracer_project/sky/dome/field.bmp";

    std::optional<dimensions> dims = read_bmp_size(file_name);
    if (not dims.has_value()) {
        std::cout << "File not found" << std::endl;
        return EXIT_FAILURE;
    }
    std::vector<std::vector<rt::color>> matrix(dims.value().width, std::vector<rt::color>(dims.value().height));
    const bool read_success = read_bmp(file_name, matrix);

    if (not read_success) {
        return EXIT_FAILURE;
    }

    std::cout << "Hello" << std::endl;

    /* Screen dimensions */
    //int width = 1366, height = 768;
    int width = 1920, height = 1080;

    rt::screen scr(width, height);

    std::cout << "Screen created" << std::endl;

    const unsigned int nbpix = 4 * dims.value().width * dims.value().height;
    std::vector<char> pixels(nbpix);

    std::cout << "Pixel vector created" << std::endl;
    
    for (int j = 0; j < dims.value().height; j++) { 
        for (int i = 0; i < dims.value().width; i++) {
            rt::color& c = matrix[i][j];
            char r = c.get_red();
            char g = c.get_green();
            char b = c.get_blue();
            const int index = 4 * (j * dims.value().width + i);
            pixels[index]     = b;
            pixels[index + 1] = g;
            pixels[index + 2] = r;
            pixels[index + 3] = 255;
        }
    }

    std::cout << "Pixels created" << std::endl;

    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom((void*) pixels.data(),
            dims.value().width,
            dims.value().height,
            32,
            sizeof(char),
            0, 0, 0, 0);
    if (surface == NULL) {
        std::cout << "Error" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Surface created" << std::endl;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(scr.renderer, surface);

    std::cout << "Texture created" << std::endl;

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
    
    SDL_Rect srcrect;
    SDL_Rect dstrect;

    srcrect.x = 0;
    srcrect.y = 0;
    srcrect.w = dims.value().width;
    srcrect.h = dims.value().height;
    dstrect.x = 0;
    dstrect.y = 0;
    dstrect.w = scr.width();
    dstrect.h = scr.height();

    std::cout << "Rectangles created" << std::endl;

    SDL_RenderClear(scr.renderer);

    SDL_RenderCopy(scr.renderer, texture, &srcrect, &dstrect);
    std::cout << "Render copy" << std::endl;

    SDL_RenderPresent(scr.renderer);

    std::cout << "Render present" << std::endl;

    SDL_Event event;
    const uint64_t time_init = get_time();
    unsigned int frame_cpt = 0;
    while(true) {
        while(SDL_PollEvent(&event)) {
                    
            if (event.type == SDL_QUIT ||
                (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)) {
                const uint64_t curr_time = get_time();
                std::cout << "Average fps: " << (1000.0 * frame_cpt) / (curr_time - time_init) << std::endl;
                return EXIT_SUCCESS;
            }
        }
        SDL_RenderClear(scr.renderer);
        SDL_RenderCopy(scr.renderer, texture, &srcrect, &dstrect);
        SDL_RenderPresent(scr.renderer);
        frame_cpt ++;
    }

}
#if 0
    mouse_pos mouse;
    mouse.set(scr.width()/2, scr.height()/2);

    const rt::color bg_col(0, 0, 0);
    const rt::color cursor_col(255, 0, 0);
    // update_screen_cursor(scr, bg_col, cursor_col, mouse);
    update_screen(scr, matrix, mouse);

    bool stop = false;

    const uint64_t time_init = get_time();
    uint64_t last_update_time = time_init;

    unsigned int frame_cpt = 0;
    
    /* Update loop */

    while (not stop) {
        const uint64_t time = get_time();

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

        /* Update every 16ms, to target 60fps */
        if (time - last_update_time >= 1000 / 60) {
            // update_screen_cursor(scr, bg_col, cursor_col, mouse);
            update_screen(scr, matrix, mouse);
            scr.update();
            last_update_time = time;
            frame_cpt ++;
        }
    }

    const uint64_t curr_time = get_time();

    std::cout << "Average fps: " << (1000.0 * frame_cpt) / (curr_time - time_init) << std::endl;

    return EXIT_SUCCESS;
}
#endif