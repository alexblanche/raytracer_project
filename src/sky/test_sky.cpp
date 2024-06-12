#include <iostream>

#include "file_readers/bmp_reader.hpp"
#include "screen/screen.hpp"

#include <algorithm>
// #include <thread>
// #include <mutex>
#include <functional>
#include <chrono>

// #include<unistd.h>

#include <cmath>

#include "light/vector.hpp"

#define PI 3.14159265359f
#define PIOVER2 1.57079632679f

/* Prototype: attempt at a real-time skydome, to be cleaned-up later */

/* Returns the current time in milliseconds */
uint64_t get_time() {
    return duration_cast< std::chrono::milliseconds >(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

struct mouse_pos {
    const float width;
    const float height;
    float theta, phi;

    mouse_pos(int width, int height)
        : width(width), height(height),
          theta(0), phi(0.5) {}

    // xr and yr are relative positions
    void set(int xr, int yr) {
        // Mouse to the right = decreasing theta
        theta -= xr / width;
        // Mouse to the top = increasing phi
        phi += yr / height;

        // theta is circular in [-PI, PI]
        if (theta > PI) {
            theta -= 2 * PI;
        }
        else if (theta < -PI) {
            theta += 2 * PI;
        }
        
        // phi is capped between -PI/2 and PI/2
        if (phi > PIOVER2) {
            phi = PIOVER2 - 1e-07f;
        }
        else if (phi < -PIOVER2) {
            phi = -PIOVER2 + 1e-07f;;
        }
    }
};

/*
void update_screen(rt::screen& scr, std::vector<std::vector<rt::color>>& image, mouse_pos&) { //mouse) {
    scr.clear();
    const float ratio_x = ((float) image.size()) / (scr.width()-1);
    const float ratio_y = ((float) image[0].size()) / (scr.height()-1);
    for (int i = 0; i < scr.width()-1; i++) {
        for (int j = 0; j < scr.height()-1; j++) {
            scr.set_pixel(i, j, image[(int) (ratio_x * i)][(int) (ratio_y * j)]);
        }
    }
}
*/

// Index of the pixel in the original texture corresponding to the index i, j of the screen
/*
int index_source (const int i, const int j, const float& ratio_x, const float& ratio_y, const int img_width) {

    // const int a = 3 * (((int) (1 * j)) * img_width + ((int) (ratio_x * i)));
    // if (a < 0 || a >= (22118400-2)) {
    //     // std::cout << "Index overflow " << a << std::endl;
    //     return 0;
    // }

    // Copying the image onto the screen
    return 3 * (j * img_width + ((int) (1 * i)));
}
*/

/** Trigonometry calculations **/

// Struct containing the X and Y axes of the screen in world space
struct screen_axes {
    rt::vector screen_x_axis;
    rt::vector screen_y_axis;

    screen_axes() {
        screen_x_axis = rt::vector(1, 0, 0);
        screen_y_axis = rt::vector(0, 1, 0);
    }

    void set(const float& theta, const float& phi) {
        const float sinphi = sin(phi);
        const float costheta = cos(theta);
        const float sintheta = sin(theta);
        screen_x_axis.x = sintheta;
        screen_x_axis.z = costheta;
        screen_y_axis.x = -sinphi * costheta;
        screen_y_axis.y = cos(phi);
        screen_y_axis.z = sinphi * sintheta;
    }
};

// Returns the catersian coordinates of the center of the screen (x0, y0, z0)
// The 4 edges of the screen plane are set at radius 1, so the center is
// set at a distance cos(fov_x)
rt::vector get_center(const float& fov_x, const float& theta, const float& phi) {
    const float l = cos(fov_x * PI);
    const float lcosphi = l * cos(phi);
    return rt::vector(
        lcosphi * cos(theta),
        l * sin(phi),
        lcosphi * (-sin(theta))
    );
}

// Returns the world space cartesian coordinates of the pixel of coordinates (i, j) (in screen space)
// rt::vector get_cartesian(screen_axes& axes,
//     const float& fov_x, const float& fov_y, const int scr_width, const int scr_height,
//     const float& theta, const float& phi, const int i, const int j) {

//     const rt::vector center = get_center(fov_x, theta, phi);
//     axes.set(theta, phi);
//     const int i0 = scr_width / 2;
//     const int j0 = scr_height / 2;
//     return center
//         + axes.screen_x_axis * (i - i0) * (fov_x * PI / i0)  // atomic horizontal angle: fov_x * pi / (width / 2)
//         + axes.screen_y_axis * (j - j0) * (fov_y * PI / j0); // atomic vertical angle: fov_y * pi / (height / 2)
// }

// Struct for spherical coordinates
struct spherical {
    float theta;
    float phi;

    // Returns the world space spherical coordinates (theta, phi) of the point of cartesian coordinates (x, y, z)
    // (assuming sqrt(x*x + y*y + z*z) < 1)
    spherical(const rt::vector& u) {

        if (u.x > 0) {
            theta = atanf(u.z / u.x) + 3.0f * PIOVER2;
        }
        else if (u.x < 0) {
            theta = atanf(u.z / u.x) + PIOVER2;
        }
        else {
            theta = 3.0f * PIOVER2;
        }

        phi = asinf(u.y / u.norm()) + PIOVER2;
    }
};



int main(int, char**) {

    /* Skydome texture */
    const char* file_name =
        //"../../../raytracer_project/sky/dome/test.bmp";
        "../../../raytracer_project/sky/dome/field.bmp";

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

    /* Screen dimensions */
    //int width = 1366, height = 768;
    int width = 1920, height = 1080;

    rt::screen scr(width, height);

    SDL_SetWindowFullscreen(scr.window, SDL_WINDOW_FULLSCREEN);
    SDL_ShowCursor(SDL_DISABLE);

    const unsigned int depth = 3;
    const unsigned int nbpix = depth * dims.value().width * dims.value().height;
    std::vector<char> pixels(nbpix);
    
    for (int j = 0; j < dims.value().height; j++) { 
        for (int i = 0; i < dims.value().width; i++) {
            rt::color& c = matrix[i][j];
            char r = c.get_red();
            char g = c.get_green();
            char b = c.get_blue();
            const int index = depth * (j * dims.value().width + i);
            pixels[index]     = b;
            pixels[index + 1] = g;
            pixels[index + 2] = r;
        }
    }

    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom((void*) pixels.data(),
            dims.value().width,
            dims.value().height,
            8 * depth, // depth bytes per pixel (in bits)
            depth * dims.value().width, // depth * width
            0, 0, 0, 0);
    if (surface == NULL) {
        std::cout << "Error" << std::endl;
        return EXIT_FAILURE;
    }

    // SDL_Texture* texture = SDL_CreateTextureFromSurface(scr.renderer, surface);


    // SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
    
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

    SDL_RenderClear(scr.renderer);

    // SDL_RenderCopy(scr.renderer, texture, &srcrect, &dstrect);
    

    // SDL_RenderPresent(scr.renderer);

    SDL_Texture* txt = SDL_CreateTexture(scr.renderer, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING, scr.width(), scr.height());

    mouse_pos mouse(scr.width(), scr.height());

    SDL_Event event;
    const uint64_t time_init = get_time();
    unsigned int frame_cpt = 0;
    char* texture_pixels;
    int texture_pitch;
    char* orig_pixels = (char*) surface->pixels;

    const int img_width = dims.value().width;
    const int img_height = dims.value().height;
    
    // const float ratio_x = ((float) (img_width - 1)) / (scr.width()-1);
    // const float ratio_y = ((float) (img_height - 1)) / (scr.height()-1);

    const int half_scr_width = scr.width() / 2;
    const int half_scr_height = scr.height() / 2;

    const float fov_x = 0.3;
    const float fov_y = 0.15;

    screen_axes axes;

    const float x_step = fov_x * PI / half_scr_width;
    const float y_step = fov_y * PI / half_scr_height;

    // Scaling factor to convert [-pi, pi] (resp. [-pi/2, pi/2]) to [0, img_width-1] (resp. [0, img_height-1])
    const float img_scale_x = (img_width - 1) / (2 * PI);
    const float img_scale_y = (img_height - 1) / PI;

    while(true) {
        while(SDL_PollEvent(&event)) {
                    
            if (event.type == SDL_MOUSEMOTION) {
                // std::cout << event.motion.xrel << ' ' << event.motion.yrel << std::endl;
                mouse.set(event.motion.xrel, event.motion.yrel);
                SDL_WarpMouseInWindow(scr.window, scr.width() >> 1, scr.height() >> 1);
            }
            else if (event.type == SDL_QUIT ||
                (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)) {
                const uint64_t curr_time = get_time();
                std::cout << "Average fps: " << (1000.0 * frame_cpt) / (curr_time - time_init) << std::endl;
                return EXIT_SUCCESS;
            }
        }

        // std::cout << "Theta = " << mouse.theta << "; Phi = " << mouse.phi << std::endl;

        // Render a new frame

        // Pre-computation of the cartesian coordinates of the pixel in world space
        const rt::vector center = get_center(fov_x, mouse.theta, mouse.phi);
        axes.set(mouse.theta, mouse.phi);
        const rt::vector scaled_x_axis = axes.screen_x_axis * x_step;
        const rt::vector scaled_y_axis = axes.screen_y_axis * y_step;

        SDL_LockTexture(txt, NULL, (void**) &texture_pixels, &texture_pitch);
        for (int j = 0; j < scr.height(); j++) {

            // Pre-computation of the cartesian coordinates of the pixel in world space
            const rt::vector y_component = scaled_y_axis * (j - half_scr_height);
            const rt::vector pre_cartesian = center + y_component;

            for (int i = 0; i < scr.width(); i++) {
                
                // Determining the cartesian coordinates of the pixel in world space
                const rt::vector x_component = scaled_x_axis * (i - half_scr_width);
                const rt::vector cartesian = pre_cartesian + x_component;
                // Converting the coordinates into spherical coordinates in world space
                const spherical sph(cartesian);
                
                /*
                const int x = (sph.theta * img_scale_x);
                const int y = (sph.phi * img_scale_y);
                if (x < 0 || x >= img_width || y < 0 || y >= img_height) {
                    std::cout << "img_width = " << img_width << "; img_height = " << img_height << std::endl;
                    std::cout << "x = " << x << "; y = " << y << std::endl;
                    std::cout << "float x = " << (sph.theta * img_scale_x) << "; float y = " << (sph.phi * img_scale_y) << std::endl;
                    std::cout << std::endl;
                    std::cout << "2 * pi = " << 2 * PI << std::endl;
                    std::cout << "theta = " << sph.theta << "; phi = " << sph.phi << std::endl;
                    return EXIT_FAILURE;
                }
                */

                // Reading the pixel of the image corresponding to the spherical coordinates of the pixel in world space
                const int index_src = 3 * (((int) (sph.phi * img_scale_y)) * img_width + (int) (sph.theta * img_scale_x));
                // Copying its color onto the screen
                const int index = 3 * (j * scr.width() + i);
                texture_pixels[index]     = orig_pixels[index_src];
                texture_pixels[index + 1] = orig_pixels[index_src + 1];
                texture_pixels[index + 2] = orig_pixels[index_src + 2];
            }

            // std::cout << std::endl;
        }
        SDL_UnlockTexture(txt);
        SDL_RenderClear(scr.renderer);
        SDL_RenderCopy(scr.renderer, txt, &srcrect, &dstrect);
        SDL_RenderPresent(scr.renderer);
        frame_cpt ++;
    }

}
