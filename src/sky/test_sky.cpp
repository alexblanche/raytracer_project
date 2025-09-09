#include <iostream>
#include "file_readers/bmp_reader.hpp"
#include "file_readers/hdr_reader.hpp"
#include "screen/screen.hpp"
#include "light/vector.hpp"
#include <chrono>
#include <cmath>

constexpr float PI = 3.14159265359f;

/* Prototype: real-time skydome, to be cleaned-up */

/* Returns the current time in milliseconds */
uint64_t get_time() {
    return duration_cast< std::chrono::milliseconds >(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

inline float absf(const float x) {
    return std::signbit(x) ? -x : x;
}


/* Struct containing the spherical coordinates controlled by the mouse */
struct mouse_pos {
    const float invwidth;
    const float invheight;
    float theta, phi;

    mouse_pos(int width, int height)
        : invwidth(1.0f / ((float) width)), invheight(1.0f / ((float) height)),
          theta(PI), phi(0) {}

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

        theta = (absf(theta) > PI) ? theta + (std::signbit(theta) ? 2*PI : -2*PI): theta;
        phi = (absf(phi) > PI/2) ? (std::signbit(phi) ? -PI/2 : PI/2) : phi;
    }
};

/** Trigonometry calculations **/

// Struct containing the X and Y axes of the screen in world space,
// and the catersian coordinates of the center of the screen (x0, y0, z0)
// The 4 edges of the screen plane are set at radius 1, so the center is
// set at a distance cos(fov_x)
struct screen_axes {
    rt::vector screen_x_axis;
    rt::vector screen_y_axis;
    rt::vector center;

    screen_axes() {
        screen_x_axis = rt::vector(1, 0, 0);
        screen_y_axis = rt::vector(0, 1, 0);
        center = rt::vector(0, 0, 1);
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

rt::vector get_cartesian(screen_axes& axes,
    const float& fov_x, const float& fov_y, const int scr_width, const int scr_height,
    const float& theta, const float& phi, const int i, const int j) {

    const rt::vector center = get_center(fov_x, theta, phi);
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
    spherical(const rt::vector& u) {

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
        //"../../../raytracer_project/sky/dome/evening.bmp";
        //"../../../raytracer_project/sky/dome/field.bmp";
        //"../../../raytracer_project/sky/dome/southern_sky.bmp";
        //"../../../raytracer_project/sky/dome/house.bmp";
        "../../../raytracer_project/sky/dome/cobblestone_street_night.bmp";

    print_bmp_info(file_name);

    std::optional<dimensions> dims = read_bmp_size(file_name);
    if (not dims.has_value()) {
        std::cout << "File not found" << std::endl;
        return EXIT_FAILURE;
    }
    const int dwidth = dims.value().width;
    const int dheight = dims.value().height;
    std::vector<std::vector<rt::color>> matrix(dwidth, std::vector<rt::color>(dheight));
    const bool read_success = read_bmp(file_name, matrix);
    if (not read_success) {
        return EXIT_FAILURE;
    }

    /* Screen dimensions */
    constexpr int width = 1920, height = 1080;
    rt::screen scr(width, height);
    SDL_SetWindowFullscreen(scr.window, SDL_WINDOW_FULLSCREEN);
    SDL_ShowCursor(SDL_DISABLE);

    /* Creation of the surface containing the colors in char format */
    constexpr unsigned int depth = 3;
    const unsigned int nbpix = depth * dwidth * dheight;
    std::vector<char> pixels(nbpix);
    
    // for (int j = 0; j < dims.value().height; j++) {

    //     const int jwidth = j * dims.value().width;

    //     for (int i = 0; i < dims.value().width; i++) {
    //         rt::color& c = matrix[i][j];
    const int dwidthdepth = dwidth * depth;
    for (int i = 0; i < dwidth; i++) {
        const std::vector<rt::color>& line = matrix[i];
        const int idepth = i * depth;
        for (int j = 0; j < dheight; j++) {
            const rt::color& c = line[j];
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
    rt::color colo(0, 0, 0);
    rt::color colo_fma(0, 0, 0);
    constexpr int nbiter = 100;
    const unsigned long int tcolo = time(0);
    for (int iter = 0; iter < nbiter; iter++) {
        for (int i = 0; i < dwidth; i++) {
            const std::vector<rt::color>& line = matrix[i];
            for (int j = 0; j < dheight; j++) {
                const rt::color& c = line[j];
                colo = (c * 2.0f) + colo;
            }
        }
    }
    const unsigned long int tcolo_end = time(0);
    for (int iter = 0; iter < nbiter; iter++) {
        for (int i = 0; i < dwidth; i++) {
            const std::vector<rt::color>& line = matrix[i];
            for (int j = 0; j < dheight; j++) {
                const rt::color& c = line[j];
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
    if (surface == NULL) {
        std::cout << "Error" << std::endl;
        return EXIT_FAILURE;
    }
    
    SDL_Rect srcrect;
    SDL_Rect dstrect;

    srcrect.x = 0;
    srcrect.y = 0;
    srcrect.w = dwidth;
    srcrect.h = dheight;
    dstrect.x = 0;
    dstrect.y = 0;
    dstrect.w = width;
    dstrect.h = height;

    /* Rendering texture */
    SDL_Texture* txt = SDL_CreateTexture(scr.renderer, SDL_PIXELFORMAT_BGR24, SDL_TEXTUREACCESS_STREAMING, width, height);

    /* Variable declarations */
    mouse_pos mouse(width, height);

    SDL_Event event;
    const uint64_t time_init = get_time();
    unsigned int frame_cpt = 0;
    
    char* texture_pixels;
    int texture_pitch;
    char* orig_pixels = (char*) surface->pixels;

    const int img_width = dwidth;
    const int img_height = dheight;

    constexpr int half_scr_width = width / 2;
    constexpr int half_scr_height = height / 2;

    constexpr float fov_x = 0.3;
    constexpr float fov_y = 0.15;

    screen_axes axes;

    constexpr float x_step = fov_x * PI / half_scr_width;
    constexpr float y_step = fov_y * PI / half_scr_height;

    // Scaling factor to convert [0, 2pi] (resp. [0, pi]) to [0, img_width-1] (resp. [0, img_height-1])
    const float img_scale_x = (img_width - 1) / (2 * PI);
    const float img_scale_y = (img_height - 1) / PI;
    
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
                    SDL_WarpMouseInWindow(scr.window, width / 2, height / 2);
                    break;
                case SDL_QUIT:
                case SDL_KEYDOWN:
                    if (event.type != SDL_KEYDOWN || event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                        const uint64_t curr_time = get_time();
                        std::cout << "Average fps: " << (1000.0 * frame_cpt) / (curr_time - time_init) << std::endl;
                        return EXIT_SUCCESS;
                    }
            }
        }
        // printf("mouse set: %d\n", cpt_mouse);

        // std::cout << "Theta = " << mouse.theta << "; Phi = " << mouse.phi << std::endl;

        /* Frame rendering */

        // Pre-computation of the cartesian coordinates of the pixel in world space
        //const rt::vector center = get_center(fov_x, mouse.theta, mouse.phi);
        axes.set(/*fov_x,*/ mouse.theta, mouse.phi);
        const rt::vector scaled_x_axis = axes.screen_x_axis * x_step;
        const rt::vector scaled_y_axis = axes.screen_y_axis * y_step;
        constexpr float tan_reset_threshold = 1.0f;

        SDL_LockTexture(txt, NULL, (void**) &texture_pixels, &texture_pitch);
        int index = 0;
        //unsigned int atan_cpt = 0;
        for (int j = 0; j < height; j++) {

            // Pre-computation of the cartesian coordinates of the pixel in world space
            // const rt::vector y_component = scaled_y_axis * (j - half_scr_height);
            // const rt::vector pre_cartesian = axes.center + y_component;
            const rt::vector pre_cartesian = fma(scaled_y_axis, j - half_scr_height, axes.center);
            // const int jwidth = j * width;

#if 0
            for (int i = 0; i < width; i++) {
                
                // Determining the cartesian coordinates of the pixel in world space
                // const rt::vector x_component = scaled_x_axis * (i - half_scr_width);
                // const rt::vector cartesian = pre_cartesian + x_component;
                const rt::vector cartesian = fma(scaled_x_axis, i - half_scr_width, pre_cartesian);
                /*
                // Converting the coordinates into spherical coordinates in world space
                const spherical sph(cartesian);

                // Reading the pixel of the image corresponding to the spherical coordinates of the pixel in world space
                const int index_src = 3 * (((int) (sph.phi * img_scale_y)) * img_width + (int) (sph.theta * img_scale_x));
                */
                
                const float theta =
                    (cartesian.x != 0) ?
                        atanf(cartesian.z / cartesian.x) + (cartesian.x > 0 ? 3.0f * (PI / 2.0f) : (PI / 2.0f))
                        :
                        3.0f * (PI / 2.0f);
        
                const float phi = asinf(cartesian.y / cartesian.norm()) + (PI / 2.0f);

                // Reading the pixel of the image corresponding to the spherical coordinates of the pixel in world space
                const int index_src = 3 * (((int) (phi * img_scale_y)) * img_width + (int) (theta * img_scale_x));
                

                // Copying its color onto the screen
                // const int index = 3 * (jwidth + i);
                // texture_pixels[index]     = orig_pixels[index_src];
                // texture_pixels[index + 1] = orig_pixels[index_src + 1];
                // texture_pixels[index + 2] = orig_pixels[index_src + 2];
                memcpy(texture_pixels + index, orig_pixels + index_src, 3);
                index += 3;
            }
#else
            /*
                cartesian.x == 0 iff
                    scaled_x_axis.x * (i - half_scr_width) + pre_cartesian.x == 0
                    i == half_scr_width - pre_cartesian.x / scaled_x_axis.x
                    if (-pre_cartesian.x / scaled_x_axis.x) is an integer
            */
            
            const float lim = half_scr_width + ((scaled_x_axis.x != 0) ? -pre_cartesian.x / scaled_x_axis.x : 1e30f);
            const bool lim_pos = 0 <= lim;
            const bool two_loops_needed = lim_pos && (lim < width);
            const bool need_limit_case = two_loops_needed && (lim - nearbyintf(lim) < 1e-5);
            const float init_cartx = fma(scaled_x_axis.x, 0 - half_scr_width, pre_cartesian.x);
            const bool starts_pos = init_cartx >= 0;
            const float const_before = starts_pos ? 3.0f * (PI / 2.0f) : (PI / 2.0f);
            const float const_after = (2.0f * PI) - const_before;
            const int lim_int = static_cast<int>(lim);
            int last_first_loop = !lim_pos ?
                width
                :
                std::max(0, std::min(lim_int + (!need_limit_case), width));
            
            rt::vector cartesian = fma(scaled_x_axis, (-1) - half_scr_width, pre_cartesian);
            
            float x = (cartesian.x != 0) ? cartesian.z / cartesian.x : (cartesian.z + 0.1 * scaled_x_axis.z) / (cartesian.x + 0.1 * scaled_x_axis.x);
            float theta = (cartesian.x != 0) ? (atanf(x) + const_before) : (starts_pos ? 0 : PI);
            float y = cartesian.y / sqrt(cartesian.x * cartesian.x + cartesian.z * cartesian.z);
            float phi = atanf(y) + (PI / 2.0f);

            // Optim? Buffering the memcpy?
            // How can I initialize index_src_last and _first correctly?
            // int index_first = index;
            // int index_src_last = -4; // so that index_src != index_src_last + 3 == -1
            // int index_src_first = -3; // so that index - index_first - 3 == index == 0
            // if (index_src != index_src_last + 3)
            //     memcpy(texture_pixels + index_first, orig_pixels + index_src_first, index - index_first - 3);
            //     index_src_first = index_src;
            // }
            // index_src_last = index_src;

            // Sol to needs_reset_theta k1, k2
            // for (0 .. k1)            deriv
            // for (k1 .. limit)        tan
            // limit case
            // for (limit + 1 .. k2)    tan
            // for (k2+1 .. width)      deriv
            // -------
            // b1 = std::min(k1, width) -> for (0 .. b1)
            // b2 = std::max(0, k1)     -> for (b2, limit)
            // b3 = std::min(k2, width) -> for (limit + 1 .. b3)
            // b4 = std::min(k2+1, width) -> for(b4, width)
            // or if no limit case: for (0 .. b1); for (b2 .. b3); for (max(k2+1, 0) .. b4)
            
            int i;
            for (i = 0; i < last_first_loop; i++) {
                cartesian += scaled_x_axis;
                
                //const float theta = atanf(cartesian.z / cartesian.x) + const_before;
                const float nx = cartesian.z / cartesian.x;
                const float dx = nx - x;
                const bool needs_reset_theta = absf(dx) > tan_reset_threshold;
                const float mx = (x + nx) * 0.5f;
                theta = needs_reset_theta ? (atanf(nx) + const_before) : theta + (dx / (1 + mx * mx));
                //if (needs_reset_theta) atan_cpt++;
                x = nx;

                //  float phi = asinf(cartesian.y / cartesian.norm()) + (PI / 2.0f);
                //const float phi = atanf(cartesian.y / sqrt(cartesian.x * cartesian.x + cartesian.z * cartesian.z)) + (PI / 2.0f);
                //if (cartesian.x * cartesian.x + cartesian.z * cartesian.z == 0) printf("First loop: div by 0\n");
                const float ny = cartesian.y / sqrt(cartesian.x * cartesian.x + cartesian.z * cartesian.z);
                const float dy = ny - y;
                const bool needs_reset_phi = absf(dy) > tan_reset_threshold;
                const float my = (y + ny) * 0.5f;
                phi = needs_reset_phi ? (atanf(y) + (PI / 2.0f)) : phi + (dy / (1 + my * my));
                //if (needs_reset_phi) atan_cpt++;
                y = ny;

                const int index_src_1 = ((static_cast<int>(phi * img_scale_y)) * img_width + static_cast<int>(theta * img_scale_x));
                const int index_src_2 = (index_src_1 << 1) + index_src_1;
                // if (index_src_r < 0) printf("FIRST LOOP index_src < 0\n");
                // if (index_src_r > max_index_src) printf("FIRST LOOP index_src >= max\n");
                //const int index_src = std::clamp(index_src_r, 0, max_index_src);
                const int index_src = std::max(0, index_src_2);
                memcpy(texture_pixels + index, orig_pixels + index_src, 3);

#ifdef DRAW_RESET
                if (needs_reset_theta || needs_reset_phi) {
                    texture_pixels[index]     = 255 * needs_reset_theta;
                    texture_pixels[index + 1] = 255 * (needs_reset_theta && needs_reset_phi);
                    texture_pixels[index + 2] = 255 * needs_reset_phi;
                }
#endif

                index += 3;
            }
            if (two_loops_needed) {
                if (need_limit_case) {
                    cartesian += scaled_x_axis;

                    theta = (theta < PI/2) || (theta > 3*PI/2) ? 0 : PI;
                    
                    //const float phi = asinf(cartesian.y / sqrt(cartesian.y * cartesian.y + cartesian.z * cartesian.z)) + (PI / 2.0f);
                    //const float phi = atanf(cartesian.y / sqrt(cartesian.x * cartesian.x + cartesian.z * cartesian.z)) + (PI / 2.0f);
                    //if (cartesian.x * cartesian.x + cartesian.z * cartesian.z == 0) printf("Limit case: div by 0\n");
                    const float ny = cartesian.y / sqrt(cartesian.x * cartesian.x + cartesian.z * cartesian.z);
                    const float dy = ny - y;
                    const bool needs_reset_phi = absf(dy) > tan_reset_threshold;
                    const float my = (y + ny) * 0.5f;
                    phi = needs_reset_phi ? (atanf(y) + (PI / 2.0f)) : phi + (dy / (1 + my * my));
                    //if (needs_reset_phi) atan_cpt++;
                    y = ny;

                    const int index_src_r = 3 * ((static_cast<int>(phi * img_scale_y)) * img_width + static_cast<int>(theta * img_scale_x));
                    // if (index_src_r < 0) printf("MID index_src < 0\n");
                    // if (index_src_r > max_index_src) printf("MID index_src >= max\n");
                    //const int index_src = std::clamp(index_src_r, 0, max_index_src);
                    const int index_src = std::max(0, index_src_r);
                    memcpy(texture_pixels + index, orig_pixels + index_src, 3);

#ifdef DRAW_RESET
                    if (needs_reset_phi) {
                        texture_pixels[index]     = 0;
                        texture_pixels[index + 1] = 0;
                        texture_pixels[index + 2] = 255 * needs_reset_phi;
                    }
#endif

                    index += 3;
                    i = lim_int + 1;
                }
                
                x = 1e30f;
                for (; i < width; i++) {
                    cartesian += scaled_x_axis;

                    //const float phi = asinf(cartesian.y / cartesian.norm()) + (PI / 2.0f);
                    //const float phi = atanf(cartesian.y / sqrt(cartesian.x * cartesian.x + cartesian.z * cartesian.z)) + (PI / 2.0f);
                    //if (cartesian.x * cartesian.x + cartesian.z * cartesian.z == 0) printf("Second loop: div by 0\n");
                    const float ny = cartesian.y / sqrt(cartesian.x * cartesian.x + cartesian.z * cartesian.z);
                    const float dy = ny - y;
                    const bool needs_reset_phi = absf(dy) > tan_reset_threshold;
                    const float my = (y + ny) * 0.5f;
                    phi = needs_reset_phi ? (atanf(y) + (PI / 2.0f)) : phi + (dy / (1 + my * my));
                    //if (needs_reset_phi) atan_cpt++;
                    y = ny;

                    // const float theta = atanf(cartesian.z / cartesian.x) + const_after;
                    // atan_cpt++;
                    const float nx = cartesian.z / cartesian.x;
                    const float dx = nx - x;
                    const bool needs_reset_theta = absf(dx) > tan_reset_threshold;
                    const float mx = (x + nx) * 0.5f;
                    theta = needs_reset_theta || needs_reset_phi ? (atanf(nx) + const_after) : theta + (dx / (1 + mx * mx));
                    //if (needs_reset_theta) atan_cpt++;
                    x = nx;

                    const int index_src_1 = ((static_cast<int>(phi * img_scale_y)) * img_width + static_cast<int>(theta * img_scale_x));
                    const int index_src_2 = (index_src_1 << 1) + index_src_1;
                    // if (index_src_r < 0) printf("SECOND LOOP index_src < 0, theta %f, phi %f, theta * img_scale_x %f, phi * img_scale_y %f, index_src %d\n",
                    //     theta, phi, theta * img_scale_x, phi * img_scale_y, index_src_r);
                    // if (index_src_r > max_index_src) printf("SECOND LOOP index_src >= max\n");
                    //const int index_src = std::clamp(index_src_r, 0, max_index_src);
                    const int index_src = std::max(0, index_src_2);
                    memcpy(texture_pixels + index, orig_pixels + index_src, 3);
                    
#ifdef DRAW_RESET
                    if (needs_reset_theta || needs_reset_phi) {
                        texture_pixels[index]     = 255 * needs_reset_theta;
                        texture_pixels[index + 1] = 255 * (needs_reset_theta && needs_reset_phi);
                        texture_pixels[index + 2] = 255 * needs_reset_phi;
                    }
#endif

                    index += 3;
                }
            }
#endif
        }
        //printf("atan per pixel : %f pcts\n", 100 * static_cast<float>(atan_cpt) / (width * height * 2));
        SDL_UnlockTexture(txt);
        SDL_RenderClear(scr.renderer);
        SDL_RenderCopy(scr.renderer, txt, &srcrect, &dstrect);
        SDL_RenderPresent(scr.renderer);
        frame_cpt ++;
    }

}
