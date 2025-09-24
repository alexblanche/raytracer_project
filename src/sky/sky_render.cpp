#include "sky/sky_render.hpp"

constexpr float PI = 3.14159265359f;
constexpr float tan_reset_threshold = 1.0f;

inline float absf(const float x) {
    return std::signbit(x) ? -x : x;
}

void render(SDL_Texture* txt, char*& texture_pixels, int& texture_pitch, char* orig_pixels,
    const int width, const int height, const int img_width,
    const rt::vector& scaled_x_axis, const rt::vector& scaled_y_axis, const rt::vector& axes_center,
    const int half_scr_width, const int half_scr_height,
    rt::screen& scr, SDL_Rect& srcrect, SDL_Rect& dstrect,
    const float img_scale_x, const float img_scale_y) {

    SDL_LockTexture(txt, NULL, (void**) &texture_pixels, &texture_pitch);
    int index = 0;
    //unsigned int atan_cpt = 0;
    for (int j = 0; j < height; j++) {

        // Pre-computation of the cartesian coordinates of the pixel in world space
        // const rt::vector y_component = scaled_y_axis * (j - half_scr_height);
        // const rt::vector pre_cartesian = axes.center + y_component;
        const rt::vector pre_cartesian = fma(scaled_y_axis, j - half_scr_height, axes_center);
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

#define POLYNOMIAL_SOLVE
#ifdef POLYNOMIAL_SOLVE
        // Computations of the solutions of abs(dx) = threshold
        // const float u0 = cartesian.z;
        // const float v0 = cartesian.x;
        // const float du = scaled_x_axis.z;
        // const float dv = scaled_x_axis.x;
        // const float a = dv * dv;
        // const float b = a + 2 * dv * v0;
        // const float c = v0 * dv + v0 * v0;
        // const float d = v0 * du - u0 * dv;
        // const float M = tan_reset_threshold;
        // const float delta = b * b - 4 * a * (c - d/M);
        const float a = scaled_x_axis.x * scaled_x_axis.x;
        const float b = a + 2 * scaled_x_axis.x * cartesian.x;
        const float c = cartesian.x * (scaled_x_axis.x + cartesian.x);
        const float d = cartesian.x * scaled_x_axis.z - cartesian.z * scaled_x_axis.x;
        const float bsq = b * b;
        const float foura = 4 * a;
        const float doverM = d / tan_reset_threshold;
        const float delta = bsq - foura * (c - doverM);
        
        int k1 = -1;
        int k2 = -1;
        const float twoa = 2 * a;
        if (delta > 0) {
            const float sqrtdelta = sqrt(delta);
            k1 = static_cast<int>((-b - sqrtdelta) / twoa);
            k2 = static_cast<int>((-b + sqrtdelta) / twoa);

            // if (k1 > k2) {
            //     exit(EXIT_FAILURE);
            // }
        }
        
        const int bound1 = (-b-a) / twoa;
        const int bound2 = (-b+a) / twoa;
        if (delta < 0 || (k1 >= bound1 && k1 <= bound2) || (k2 >= bound1 && k2 <= bound2)) {
            // Replace a, b, c with -a, -b, -c
            float alt_delta = std::max(bsq - foura * (c + doverM), 0.0f);
            const float sqrtaltdelta = sqrt(alt_delta);
            k1 = static_cast<int>(-(b + sqrtaltdelta) / twoa);
            k2 = static_cast<int>(-(b - sqrtaltdelta) / twoa);
            // if (k1 > k2) {
            //     const int temp = k1;
            //     k1 = k2;
            //     k2 = temp;
            // }
        }

        if (!(k2 < 0 || k1 >= width)) {
        // if ((k1 >= 0 && k1 < width) || (k2 >= 0 && k2 < width)) {
            //const int index_start = index;
            const int b1 = std::min(k1, width);
            const int b3 = std::min(k2, width);
            const int b2 = std::min(b3, lim_int + (!need_limit_case));

//#define TEST_SPLIT
#ifdef TEST_SPLIT
            int i;
            for (i = 0; i < b1; i++) {
                // deriv
                texture_pixels[index] = 0; texture_pixels[index + 1] = 0; texture_pixels[index + 2] = 255; // Red
                index += 3;
            }
            for (; i < b2; i++) {
                // tan (before)
                texture_pixels[index] = 255; texture_pixels[index + 1] = 0; texture_pixels[index + 2] = 0; // Blue
                index += 3;
            }
            if (need_limit_case) {
                texture_pixels[index] = 255; texture_pixels[index + 1] = 255; texture_pixels[index + 2] = 255; // White
                index += 3;
                i++;
            }
            for (; i < b3; i++) {
                // tan (after)
                texture_pixels[index] = 0; texture_pixels[index + 1] = 255; texture_pixels[index + 2] = 0; // Green
                index += 3;
            }
            for (; i < width; i++) {
                // deriv
                texture_pixels[index] = 0; texture_pixels[index + 1] = 255; texture_pixels[index + 2] = 255; // Yellow
                index += 3;
            }
            continue;
#else

            int i;
            for (i = 0; i < b1; i++) {
                // deriv
                cartesian += scaled_x_axis;
            
                const float nx = cartesian.z / cartesian.x;
                const float dx = nx - x;
                const float mx = (x + nx) * 0.5f;
                theta += dx / (1 + mx * mx);
                x = nx;

                const float ny = cartesian.y / sqrt(cartesian.x * cartesian.x + cartesian.z * cartesian.z);
                const float dy = ny - y;
                const bool needs_reset_phi = absf(dy) > tan_reset_threshold;
                const float my = (y + ny) * 0.5f;
                phi = needs_reset_phi ? (atanf(y) + (PI / 2.0f)) : phi + (dy / (1 + my * my));
                y = ny;

                const int index_src_1 = ((static_cast<int>(phi * img_scale_y)) * img_width + static_cast<int>(theta * img_scale_x));
                const int index_src_2 = (index_src_1 << 1) + index_src_1;
                const int index_src = std::max(0, index_src_2);
                memcpy(texture_pixels + index, orig_pixels + index_src, 3);

                index += 3;
            }
            // const int i0 = i;
            for (; i < b2; i++) {
                // tan (before)
                cartesian += scaled_x_axis;
            
                const float nx = cartesian.z / cartesian.x;
                theta = atanf(nx) + const_before;
                x = nx;

                const float ny = cartesian.y / sqrt(cartesian.x * cartesian.x + cartesian.z * cartesian.z);
                const float dy = ny - y;
                const bool needs_reset_phi = absf(dy) > tan_reset_threshold;
                const float my = (y + ny) * 0.5f;
                phi = needs_reset_phi ? (atanf(y) + (PI / 2.0f)) : phi + (dy / (1 + my * my));
                y = ny;

                const int index_src_1 = ((static_cast<int>(phi * img_scale_y)) * img_width + static_cast<int>(theta * img_scale_x));
                const int index_src_2 = (index_src_1 << 1) + index_src_1;
                const int index_src = std::max(0, index_src_2);
                memcpy(texture_pixels + index, orig_pixels + index_src, 3);

                // if (i == i0 || i == b2 - 1) {
                //     texture_pixels[index]     = 0;
                //     texture_pixels[index + 1] = 255;
                //     texture_pixels[index + 2] = 255;
                // }

                index += 3;
            }
            if (need_limit_case) {
                
                cartesian += scaled_x_axis;

                theta = (theta < PI/2) || (theta > 3*PI/2) ? 0 : PI;
                
                const float ny = cartesian.y / sqrt(cartesian.x * cartesian.x + cartesian.z * cartesian.z);
                const float dy = ny - y;
                const bool needs_reset_phi = absf(dy) > tan_reset_threshold;
                const float my = (y + ny) * 0.5f;
                phi = needs_reset_phi ? (atanf(y) + (PI / 2.0f)) : phi + (dy / (1 + my * my));
                y = ny;

                const int index_src_r = 3 * ((static_cast<int>(phi * img_scale_y)) * img_width + static_cast<int>(theta * img_scale_x));
                const int index_src = std::max(0, index_src_r);
                memcpy(texture_pixels + index, orig_pixels + index_src, 3);

                // texture_pixels[index]     = 0;
                // texture_pixels[index + 1] = 0;
                // texture_pixels[index + 2] = 255;

                index += 3;
                i = lim_int + 1;
            }
            //const bool blue = i == 0;
            //if (j == 0) printf("theta %f\n", theta);
            const float const_correct = (!two_loops_needed) ? const_before : const_after;
            //const int i0 = i;
            for (; i < b3; i++) {
                // tan (after)
                cartesian += scaled_x_axis;
            
                x = cartesian.z / cartesian.x;
                theta = atanf(x) + const_correct;//(i==i0 && !need_limit_case ? const_before : const_correct); // UGLY, but correct. Proper fix to be found

                const float ny = cartesian.y / sqrt(cartesian.x * cartesian.x + cartesian.z * cartesian.z);
                const float dy = ny - y;
                const bool needs_reset_phi = absf(dy) > tan_reset_threshold;
                const float my = (y + ny) * 0.5f;
                phi = needs_reset_phi ? (atanf(y) + (PI / 2.0f)) : phi + (dy / (1 + my * my));
                y = ny;

                const int index_src_1 = ((static_cast<int>(phi * img_scale_y)) * img_width + static_cast<int>(theta * img_scale_x));
                const int index_src_2 = (index_src_1 << 1) + index_src_1;
                const int index_src = std::max(0, index_src_2);
                memcpy(texture_pixels + index, orig_pixels + index_src, 3);

                // if (i == b2 || i == b2 + 1 || i == b3 - 1) {
                //     texture_pixels[index]     = 0;
                //     texture_pixels[index + 1] = 0;
                //     texture_pixels[index + 2] = 255;
                // }
                // if (blue) {
                //     texture_pixels[index] = 255;
                // }

                index += 3;
            }
            for (; i < width; i++) {
                // deriv
                cartesian += scaled_x_axis;

                const float ny = cartesian.y / sqrt(cartesian.x * cartesian.x + cartesian.z * cartesian.z);
                const float dy = ny - y;
                const bool needs_reset_phi = absf(dy) > tan_reset_threshold;
                const float my = (y + ny) * 0.5f;
                phi = needs_reset_phi ? (atanf(y) + (PI / 2.0f)) : phi + (dy / (1 + my * my));
                y = ny;

                const float nx = cartesian.z / cartesian.x;
                const float dx = nx - x;
                const float mx = (x + nx) * 0.5f;
                theta = needs_reset_phi ? (atanf(nx) + const_correct) : theta + (dx / (1 + mx * mx));
                x = nx;

                const int index_src_1 = ((static_cast<int>(phi * img_scale_y)) * img_width + static_cast<int>(theta * img_scale_x));
                const int index_src_2 = (index_src_1 << 1) + index_src_1;
                const int index_src = std::max(0, index_src_2);
                memcpy(texture_pixels + index, orig_pixels + index_src, 3);

                index += 3;
            }
#endif
        }
        else {
            // one loop
            for (int i = 0; i < last_first_loop; i++) {
                cartesian += scaled_x_axis;
                
                const float nx = cartesian.z / cartesian.x;
                const float dx = nx - x;
                const float mx = (x + nx) * 0.5f;
                theta += dx / (1 + mx * mx);
                x = nx;

                const float ny = cartesian.y / sqrt(cartesian.x * cartesian.x + cartesian.z * cartesian.z);
                const float dy = ny - y;
                const bool needs_reset_phi = absf(dy) > tan_reset_threshold;
                const float my = (y + ny) * 0.5f;
                phi = needs_reset_phi ? (atanf(y) + (PI / 2.0f)) : phi + (dy / (1 + my * my));
                y = ny;

                const int index_src_1 = ((static_cast<int>(phi * img_scale_y)) * img_width + static_cast<int>(theta * img_scale_x));
                const int index_src_2 = (index_src_1 << 1) + index_src_1;
                const int index_src = std::max(0, index_src_2);
                memcpy(texture_pixels + index, orig_pixels + index_src, 3);

                // if (starts_pos) {
                //     texture_pixels[index] = 255;
                // }
                // texture_pixels[index + 2] = 255;
                
//#define DRAW_RESET
#ifdef DRAW_RESET
                if (needs_reset_theta || needs_reset_phi) {
                    texture_pixels[index]     = 255 * needs_reset_theta;
                    texture_pixels[index + 1] = 255 * (needs_reset_theta && needs_reset_phi);
                    texture_pixels[index + 2] = 255 * needs_reset_phi;
                }
#endif
#define SHOULD_BE_POLY
#ifdef SHOULD_BE_POLY
                // if (needs_reset_theta) {
                //     texture_pixels[index]     = 0;
                //     texture_pixels[index + 1] = 128;
                //     texture_pixels[index + 2] = 255;
                // }
#endif

                index += 3;
            }
        }
#else
        
        int i;
//#define BUFFER_CPY
#ifdef BUFFER_CPY
        int init_index = index;
        int init_index_src = 0;
        int prev_index_src = -2;
        int cpt = 0;
#endif
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
#ifndef BUFFER_CPY
            memcpy(texture_pixels + index, orig_pixels + index_src, 3);
#else
            if (index_src != prev_index_src + 3) {
                // if (cpt != 3) {
                //     for (int k = 0; k < cpt; k += 3) {
                //         texture_pixels[init_index + k]     = 0;
                //         texture_pixels[init_index + k + 1] = 0;
                //         texture_pixels[init_index + k + 2] = 255;
                //     }
                // }
                // else {
                    memcpy(texture_pixels + init_index, orig_pixels + init_index_src, cpt);
                    cpt = 0;
                //}
                init_index = index;
                init_index_src = index_src;
            }
            prev_index_src = index_src;
            cpt += 3;
#endif
//#define DRAW_RESET
#ifdef DRAW_RESET
            if (needs_reset_theta || needs_reset_phi) {
                texture_pixels[index]     = 255 * needs_reset_theta;
                texture_pixels[index + 1] = 255 * (needs_reset_theta && needs_reset_phi);
                texture_pixels[index + 2] = 255 * needs_reset_phi;
            }
#endif
//#define SHOULD_BE_POLY
#ifdef SHOULD_BE_POLY
            if (needs_reset_theta) {
                texture_pixels[index]     = 0;
                texture_pixels[index + 1] = 128;
                texture_pixels[index + 2] = 255;
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
#ifndef BUFFER_CPY
                memcpy(texture_pixels + index, orig_pixels + index_src, 3);
#else
                if (index_src != prev_index_src + 3) {
                    // if (cpt != 3) {
                    //     for (int k = 0; k < cpt; k += 3) {
                    //         texture_pixels[init_index + k]     = 0;
                    //         texture_pixels[init_index + k + 1] = 0;
                    //         texture_pixels[init_index + k + 2] = 255;
                    //     }
                    // }
                    // else {
                        memcpy(texture_pixels + init_index, orig_pixels + init_index_src, cpt);
                        cpt = 0;
                    //}
                    init_index = index;
                    init_index_src = index_src;
                }
                prev_index_src = index_src;
                cpt += 3;
#endif

#ifdef DRAW_RESET
                if (needs_reset_phi) {
                    texture_pixels[index]     = 0;
                    texture_pixels[index + 1] = 0;
                    texture_pixels[index + 2] = 255;
                }
#endif
#ifdef SHOULD_BE_POLY
                texture_pixels[index]     = 0;
                texture_pixels[index + 1] = 128;
                texture_pixels[index + 2] = 255;
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
#ifndef BUFFER_CPY
                memcpy(texture_pixels + index, orig_pixels + index_src, 3);
#else
                if (index_src != prev_index_src + 3) {
                    // if (cpt != 3) {
                    //     for (int k = 0; k < cpt; k += 3) {
                    //         texture_pixels[init_index + k]     = 0;
                    //         texture_pixels[init_index + k + 1] = 0;
                    //         texture_pixels[init_index + k + 2] = 255;
                    //     }
                    // }
                    // else {
                        memcpy(texture_pixels + init_index, orig_pixels + init_index_src, cpt);
                        cpt = 0;
                    //}
                    init_index = index;
                    init_index_src = index_src;
                }
                prev_index_src = index_src;
                cpt += 3;
#endif
                
#ifdef DRAW_RESET
                if (needs_reset_theta || needs_reset_phi) {
                    texture_pixels[index]     = 255 * needs_reset_theta;
                    texture_pixels[index + 1] = 255 * (needs_reset_theta && needs_reset_phi);
                    texture_pixels[index + 2] = 255 * needs_reset_phi;
                }
#endif
#ifdef SHOULD_BE_POLY
                if (needs_reset_theta) {
                    texture_pixels[index]     = 0;
                    texture_pixels[index + 1] = 128;
                    texture_pixels[index + 2] = 255;
                }
#endif

                index += 3;
            }

#ifdef BUFFER_CPY
            
            memcpy(texture_pixels + init_index, orig_pixels + init_index_src, cpt);
#endif
        }
#endif
#endif
    }
    //printf("atan per pixel : %f pcts\n", 100 * static_cast<float>(atan_cpt) / (width * height * 2));
    SDL_UnlockTexture(txt);
    SDL_RenderClear(scr.renderer);
    SDL_RenderCopy(scr.renderer, txt, &srcrect, &dstrect);
    SDL_RenderPresent(scr.renderer);

}


// BUFFER_CPY: bad idea, goes from 26fps to 20fps...
// POLYNOMIAL_SOLVE: same