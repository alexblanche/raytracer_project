#include "sky/bmp_reader.hpp"
#include "sky/screen.hpp"
#include "sky/vector.hpp"
#include "sky/sky_render.hpp"

#include <iostream>
#include <chrono>
#include <cmath>

/* Prototype: real-time skydome, to be cleaned-up */

/* Returns the current time in milliseconds */
uint64_t get_time() {
    return duration_cast< std::chrono::milliseconds >(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

// inline float absf(const float x) {
//     return std::signbit(x) ? -x : x;
// }


/* Struct containing the spherical coordinates controlled by the mouse */
struct mouse_pos {
    const float invwidth;
    const float invheight;
    float theta, phi;

    mouse_pos(int width, int height)
        : invwidth(1.0f / ((float) width)), invheight(1.0f / ((float) height)),
          theta(Pi), phi(0) {}

    // xr and yr are relative positions
    void set(int xr, int yr) {
        // Mouse to the right = decreasing theta
        theta -= xr * invwidth;
        // Mouse to the top = increasing phi
        phi += yr * invheight;

        // theta is circular in [-PI, PI]
        // if (theta > PI) {
        //     theta -= 2 * PI;
        // }
        // else if (theta < -PI) {
        //     theta += 2 * PI;
        // }
        
        // // phi is capped between -PI/2 and PI/2
        // if (phi > (PI / 2.0f)) {
        //     phi = (PI / 2.0f);
        // }
        // else if (phi < -(PI / 2.0f)) {
        //     phi = -(PI / 2.0f);
        // }

        theta = (std::abs(theta) > Pi)     ? theta + (std::signbit(theta) ? 2 * Pi : -2 * Pi): theta;
        phi   = (std::abs(phi)   > Pi / 2) ? (std::signbit(phi) ? -Pi / 2 : Pi / 2) : phi;
    }
};

/** Trigonometry calculations **/

// Struct containing the X and Y axes of the screen in world space,
// and the catersian coordinates of the center of the screen (x0, y0, z0)
// The 4 edges of the screen plane are set at radius 1, so the center is
// set at a distance cos(fov_x)
struct screen_axes {
    sky::vector screen_x_axis;
    sky::vector screen_y_axis;
    sky::vector center;

    screen_axes() {
        screen_x_axis = sky::vector(1, 0, 0);
        screen_y_axis = sky::vector(0, 1, 0);
        center        = sky::vector(0, 0, 1);
    }

    void set(/*const float fov_x,*/ const float theta, const float phi) {
        const float cosphi = cosf(phi);
        const float sinphi = sinf(phi);
        const float costheta = cosf(theta);
        const float sintheta = sinf(theta);
        // float cosphi, sinphi, costheta, sintheta;
        // sincosf(phi, &sinphi, &cosphi);
        // sincosf(theta, &sintheta, &costheta);

        //const float l = cosf(fov_x * PI);
        constexpr float l = 0.58778525229f; //cosf(0.3f * PI);

        screen_x_axis.x = sintheta;
        screen_x_axis.z = costheta;

        screen_y_axis.x = -sinphi * costheta;
        screen_y_axis.y = cosphi;
        screen_y_axis.z = sinphi * sintheta;

        const float lcosphi = l * cosphi;
        center.x = lcosphi * costheta;
        center.y = l * sinphi;
        center.z = lcosphi * (-sintheta);
    }
};

/*
// Returns the world space cartesian coordinates of the pixel of coordinates (i, j) (in screen space)
// Prototype: the computations have been placed into the code at the right place, to optimize pre-computation

sky::vector get_cartesian(screen_axes& axes,
    const float& fov_x, const float& fov_y, const int scr_width, const int scr_height,
    const float& theta, const float& phi, const int i, const int j) {

    const sky::vector center = get_center(fov_x, theta, phi);
    axes.set(theta, phi);
    const int i0 = scr_width / 2;
    const int j0 = scr_height / 2;
    return center
        + axes.screen_x_axis * (i - i0) * (fov_x * PI / i0)  // atomic horizontal angle: fov_x * pi / (width / 2)
        + axes.screen_y_axis * (j - j0) * (fov_y * PI / j0); // atomic vertical angle: fov_y * pi / (height / 2)
}
*/

// Struct for spherical coordinates of pixels in world space
#if 0
struct spherical {
    float theta;
    float phi;

    // Returns the world space spherical coordinates (theta, phi) of the point of cartesian coordinates (x, y, z)
    // (assuming sqrt(x*x + y*y + z*z) < 1)
    spherical(const sky::vector& u) {

        // OBSOLETE : inlined in render loop
        if (u.x > 0) {
            theta = atanf(u.z / u.x) + 3.0f * (PI / 2.0f);
        }
        else if (u.x < 0) {
            theta = atanf(u.z / u.x) + (PI / 2.0f);
        }
        else {
            theta = 3.0f * (PI / 2.0f);
        }

        phi = asinf(u.y / u.norm()) + (PI / 2.0f);
    }
};
#endif

int main(int argc, char** argv) {

    /* Skydome texture */
    const char* file_name =
        (argc == 2) ?
        argv[1]
        :
        "../../../assets/cobblestone_street_night.bmp";
        
    print_bmp_info(file_name);

    std::optional<dimensions> dims = read_bmp_size(file_name);
    if (not dims.has_value()) {
        std::cout << "File not found" << std::endl;
        return EXIT_FAILURE;
    }
    const int dwidth  = dims.value().width;
    const int dheight = dims.value().height;
    std::vector<std::vector<sky::color>> matrix(dwidth, std::vector<sky::color>(dheight));
    const exit_status read_success = read_bmp(file_name, matrix);
    if (read_success == exit_status::Failure) {
        return EXIT_FAILURE;
    }

    /* Screen dimensions */
    render_parameters param;
    SDL_SetWindowFullscreen(param.scr.window, SDL_WINDOW_FULLSCREEN);
    SDL_ShowCursor(SDL_DISABLE);

    /* Creation of the surface containing the colors in char format */
    constexpr unsigned int depth = 3;
    const unsigned int nbpix = depth * dwidth * dheight;
    std::vector<char> pixels(nbpix);
    
    // for (int j = 0; j < dims.value().height; j++) {

    //     const int jwidth = j * dims.value().width;

    //     for (int i = 0; i < dims.value().width; i++) {
    //         sky::color& c = matrix[i][j];
    const int dwidthdepth = dwidth * depth;
    for (int i = 0; i < dwidth; i++) {
        const std::vector<sky::color>& line = matrix[i];
        const int idepth = i * depth;
        for (int j = 0; j < dheight; j++) {
            const sky::color& c = line[j];
            const char r = c.get_red();
            const char g = c.get_green();
            const char b = c.get_blue();
            const int index = //depth * (j * dwidth + i);
                j * dwidthdepth + idepth;
            pixels[index]     = b;
            pixels[index + 1] = g;
            pixels[index + 2] = r;
        }
    }

    /////////////////////////////
    // Test fused multiply add (fma)
    // 16s classic, 11s fma
    /*
    sky::color colo(0, 0, 0);
    sky::color colo_fma(0, 0, 0);
    constexpr int nbiter = 100;
    const unsigned long int tcolo = time(0);
    for (int iter = 0; iter < nbiter; iter++) {
        for (int i = 0; i < dwidth; i++) {
            const std::vector<sky::color>& line = matrix[i];
            for (int j = 0; j < dheight; j++) {
                const sky::color& c = line[j];
                colo = (c * 2.0f) + colo;
            }
        }
    }
    const unsigned long int tcolo_end = time(0);
    for (int iter = 0; iter < nbiter; iter++) {
        for (int i = 0; i < dwidth; i++) {
            const std::vector<sky::color>& line = matrix[i];
            for (int j = 0; j < dheight; j++) {
                const sky::color& c = line[j];
                colo_fma = fma(c, (real) 2.0f, colo_fma);
            }
        }
    }
    const unsigned long int tcolo_end_end = time(0);
    printf("colo = (%f, %f, %f), colo_fma = (%f, %f, %f)\ntime colo = %lu s, colo_fma = %lu s\n", colo.get_red(), colo.get_green(), colo.get_blue(),
        colo_fma.get_red(), colo_fma.get_green(), colo_fma.get_blue(), tcolo_end - tcolo, tcolo_end_end - tcolo_end);
    */
    /////////////////////////

    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom((void*) pixels.data(),
            dwidth,
            dheight,
            8 * depth, // depth bytes per pixel (in bits)
            depth * dwidth, // depth * width
            0, 0, 0, 0);
    if (surface == nullptr) {
        std::cout << "Error" << std::endl;
        return EXIT_FAILURE;
    }

    param.scr.srcrect.x = 0;
    param.scr.srcrect.y = 0;
    param.scr.srcrect.w = dwidth;
    param.scr.srcrect.h = dheight;
    param.scr.dstrect.x = 0;
    param.scr.dstrect.y = 0;
    param.scr.dstrect.w = width;
    param.scr.dstrect.h = height;

    /* Rendering texture */
    param.txt = SDL_CreateTexture(param.scr.renderer, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING, width, height);

    /* Variable declarations */
    mouse_pos mouse(width, height);

    SDL_Event event;
    const uint64_t time_init = get_time();
    unsigned int frame_cpt = 0;
    
    param.orig_pixels = (char*) surface->pixels;

    param.img_width = dwidth;
    param.img_height = dheight;

    screen_axes axes;

    constexpr float x_step = fov_x * Pi / half_scr_width;
    constexpr float y_step = fov_y * Pi / half_scr_height;

    // Scaling factor to convert [0, 2pi] (resp. [0, pi]) to [0, img_width-1] (resp. [0, img_height-1])
    param.img_scale_x = (param.img_width  - 1) / (2 * Pi);
    param.img_scale_y = (param.img_height - 1) / Pi;
    
    //const int max_index_src = img_width * img_height * 3 - 1;

    while(true) {
        /* Event handling */

        // int cpt_mouse = 0;
        while(SDL_PollEvent(&event)) {
            
            switch (event.type) {
                case SDL_MOUSEMOTION:
                    // std::cout << event.motion.xrel << ' ' << event.motion.yrel << std::endl;
                    mouse.set(event.motion.xrel, event.motion.yrel);
                    // cpt_mouse++;
                    SDL_WarpMouseInWindow(param.scr.window, width / 2, height / 2);
                    break;
                case SDL_QUIT:
                case SDL_KEYDOWN:
                    if (event.type != SDL_KEYDOWN || event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                        const uint64_t curr_time = get_time();
                        const float fps = (1000.0f * static_cast<float>(frame_cpt)) / static_cast<float>(curr_time - time_init);
                        std::cout << "Average fps: " << fps << std::endl;
                        return EXIT_SUCCESS;
                    }
                    break;
                default:
                    break;
            }
        }
        // printf("mouse set: %d\n", cpt_mouse);

        // std::cout << "Theta = " << mouse.theta << "; Phi = " << mouse.phi << std::endl;

        /* Frame rendering */

        // Pre-computation of the cartesian coordinates of the pixel in world space
        //const sky::vector center = get_center(fov_x, mouse.theta, mouse.phi);
        axes.set(/*fov_x,*/ mouse.theta, mouse.phi);
        param.axes_center = axes.center;
        param.scaled_x_axis = axes.screen_x_axis * x_step;
        param.scaled_y_axis = axes.screen_y_axis * y_step;

        render(param);

        frame_cpt ++;
    }
}
