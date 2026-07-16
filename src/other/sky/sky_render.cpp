#include "other/sky/sky_render.hpp"

#include <iostream>

static constexpr float tan_reset_threshold = 1.0f;
static constexpr float tan_reset_threshold_phi = 40.0f;

namespace loop_version {

    enum class theta {
        UpdateThetaDerivative, UpdateThetaTestPhi,
        UpdateThetaAtanBefore, UpdateThetaAtanAfter,
        UpdateThetaBothBefore, UpdateThetaBothAfter
    };
    enum class phi {
        TestPhi, NoTestPhi
    };
    enum class limit {
        LimitCase, NotLimitCase
    };
}


template<loop_version::theta vtheta, loop_version::phi vphi, loop_version::limit vlim>
static inline void segment_loop(const render_parameters& param, segment_loop_parameters& sl_param,
    const int bound, const float const_theta) {

    using enum loop_version::theta;
    using enum loop_version::phi;
    using enum loop_version::limit;
    
    int i = sl_param.index_loop;
    for (; i < bound; i++) {

        sl_param.cartesian += param.scaled_x_axis;
        const float ny = sl_param.cartesian.y / sqrt(sl_param.cartesian.x * sl_param.cartesian.x + sl_param.cartesian.z * sl_param.cartesian.z);
        // Test: approximation of sqrt(1 + X) by (1 + 0.5f * X * (1 - 0.25f * X))
        // Sufficiently precise, but does not improve speed, even without tests ("abs(x) > abs(z)")
        // ////////
        // const float absx = std::abs(cartesian.x);
        // //const float absz = std::abs(cartesian.z);
        // //const bool casexz = absx > absz;
        // // float nx;
        // // if constexpr (NotLimitCase) {
        // //     nx = cartesian.z / cartesian.x;
        // // }
        // //const float X = casexz ? nx : cartesian.x / cartesian.z;
        // const float X = cartesian.z / cartesian.x;
        // //
        // const float X2 = X * X;
        // //const float max_xz = casexz ? absx : absz;
        // const float max_xz = absx;
        // const float ny = cartesian.y / (max_xz * fma(fma(-0.125f, X2, 0.5f), X2, 1.0f));
        // ////////

        const float dy = ny - sl_param.y;
        const float my = (sl_param.y + ny) * 0.5f;
        sl_param.y = ny;

        if constexpr (vlim == LimitCase) {
            sl_param.theta = (sl_param.theta < Pi / 2) || (sl_param.theta > 3 * Pi / 2) ? 0 : Pi;
            i = sl_param.lim_int + 1;
        }
        else if constexpr (vlim == NotLimitCase) {

            const float nx = sl_param.cartesian.z / sl_param.cartesian.x;

            if constexpr (vtheta == UpdateThetaDerivative || (vtheta == UpdateThetaTestPhi && vphi == NoTestPhi)) {
                const float dx = nx - sl_param.x;
                const float mx = (sl_param.x + nx) * 0.5f;
                sl_param.theta += dx / (1 + mx * mx);
            }
            else if constexpr (vtheta == UpdateThetaTestPhi) {
                const float dx = nx - sl_param.x;
                const float mx = (sl_param.x + nx) * 0.5f;
                const bool needs_reset_phi = std::abs(dy) > tan_reset_threshold;
                sl_param.phi   = needs_reset_phi ? (atanf(ny) + (Pi / 2.0f)) : sl_param.phi   + (dy / (1 + my * my));
                sl_param.theta = needs_reset_phi ? (atanf(nx) + const_theta) : sl_param.theta + (dx / (1 + mx * mx));
            }
            else if constexpr (vtheta == UpdateThetaAtanBefore || vtheta == UpdateThetaAtanAfter) {
                sl_param.theta = atanf(nx) + const_theta;
            }
            else if constexpr (vtheta == UpdateThetaBothBefore || vtheta == UpdateThetaBothAfter) {
                const float dx = nx - sl_param.x;
                const float mx = (sl_param.x + nx) * 0.5f;
                const bool needs_reset_theta = std::abs(dx) > tan_reset_threshold;
                sl_param.theta = needs_reset_theta ? (atanf(nx) + const_theta) : sl_param.theta + (dx / (1 + mx * mx));
            }
            sl_param.x = nx;
        }

        if constexpr (!(vlim == NotLimitCase && vtheta == UpdateThetaTestPhi && vphi != NoTestPhi)) { // phi already updated
            if constexpr (vphi == TestPhi || vlim == LimitCase) {
                const bool needs_reset_phi = std::abs(dy) > tan_reset_threshold;
                sl_param.phi = needs_reset_phi ? (atanf(ny) + (Pi / 2.0f)) : sl_param.phi + (dy / (1 + my * my));    
            }
            else if constexpr (vphi == NoTestPhi) {
                sl_param.phi += dy / (1 + my * my);
            }
        }

        const int index_src = (param.img_height - static_cast<int>(sl_param.phi * param.img_scale_y)) * param.img_width + static_cast<int>(sl_param.theta * param.img_scale_x);
        int byte_src = std::min(std::max(0, index_src), param.img_buffer_max_index) * 3;

        std::memcpy(param.texture_pixels + sl_param.index, param.orig_pixels + byte_src, 3);

        // if constexpr (vphi == TestPhi) {
        //     const bool needs_reset_phi = std::abs(dy) > tan_reset_threshold;
        //     if (needs_reset_phi)
        //         texture_pixels[index + 1] = '\255';
        //     texture_pixels[index + 2] = '\255';
        // }

        sl_param.index += 3;

        if constexpr (vlim == LimitCase) {
            break;
        }
    }

    sl_param.index_loop = i;
}

static void compute_bounds_theta_update(int& k1_io, int& k2_io,
    const sky::vector& v, const sky::vector& dv) {

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
    const float a = dv.x * dv.x;
    const float b = a + 2 * dv.x * v.x;
    const float c = v.x * (dv.x + v.x);
    const float d = v.x * dv.z - v.z * dv.x;
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
        const float alt_delta = std::max(bsq - foura * (c + doverM), 0.0f);
        const float sqrtaltdelta = sqrt(alt_delta);
        k1 = static_cast<int>(-(b + sqrtaltdelta) / twoa);
        k2 = static_cast<int>(-(b - sqrtaltdelta) / twoa);
        // if (k1 > k2) {
        //     const int temp = k1;
        //     k1 = k2;
        //     k2 = temp;
        // }
    }

    k1_io = k1;
    k2_io = k2;
}

enum class phi_test_bounds {
    AlwaysTest, NeverTest, TestBetween, TestOutside
};


[[maybe_unused]] static void print_ptb(phi_test_bounds ptb) {
    using enum phi_test_bounds;
    switch (ptb) {
        case AlwaysTest : printf("AlwaysTest\n" ); break;
        case NeverTest  : printf("NeverTest\n"  ); break;
        case TestBetween: printf("TestBetween\n"); break;
        case TestOutside: printf("TestOutside\n"); break;
        default: throw;
    }
}

// Returns true if a non-empty test of phi has to be done, between k1 and k2
// False if no solution was found
static phi_test_bounds compute_bounds_phi_update(int& k1_io, int& k2_io,
    const sky::vector& v, const sky::vector& dv) {

    using enum phi_test_bounds;
    
    // Resolution of ak^2 + bk + c < 0
    constexpr float m2 = tan_reset_threshold_phi * tan_reset_threshold_phi;
    const float a = m2 * dv.x * dv.x - dv.y * dv.y + m2 * dv.z * dv.z;
    const float b = 2.0f * (m2 * (v.x * dv.x + v.z * dv.z) - v.y * dv.y);
    const float c = m2 * (v.x * v.x + v.z * v.z) - v.y * v.y;

    const float delta = b * b - 4.0f * a * c;
    if (delta < 0.0f) {
        // a > 0: never < 0 => no test needed. a < 0 : always test
        return (a > 0.0f) ? NeverTest : AlwaysTest;
    }
    
    const float twoa = 2 * a;
    const float sqrtdelta = sqrt(delta);
    k1_io = (-b - sqrtdelta) / twoa;
    k2_io = (-b + sqrtdelta) / twoa;

    return (a > 0.0f) ? TestBetween : TestOutside;
}

void render(const render_parameters& param) {

    SDL_LockTexture(param.txt, nullptr, reinterpret_cast<void**>(&param.texture_pixels), &param.texture_pitch);

    segment_loop_parameters sl_param;
    sl_param.index = 0;

    //unsigned int atan_cpt = 0;
    for (int j = 0; j < height; j++) {

        // Pre-computation of the cartesian coordinates of the pixel in world space
        // const sky::vector y_component = scaled_y_axis * (j - half_scr_height);
        // const sky::vector pre_cartesian = axes.center + y_component;
        const sky::vector pre_cartesian = fma(param.scaled_y_axis, j - half_scr_height, param.axes_center);
        // const int jwidth = j * width;

#if 0
        for (int i = 0; i < width; i++) {
            
            // Determining the cartesian coordinates of the pixel in world space
            // const sky::vector x_component = scaled_x_axis * (i - half_scr_width);
            // const sky::vector cartesian = pre_cartesian + x_component;
            const sky::vector cartesian = fma(param.scaled_x_axis, i - half_scr_width, pre_cartesian);
            /*
            // Converting the coordinates into spherical coordinates in world space
            const spherical sph(cartesian);

            // Reading the pixel of the image corresponding to the spherical coordinates of the pixel in world space
            const int index_src = 3 * (((int) (sph.phi * img_scale_y)) * img_width + (int) (sph.theta * img_scale_x));
            */
            
            const float theta =
                (cartesian.x != 0) ?
                    atanf(cartesian.z / cartesian.x) + (cartesian.x > 0 ? 3.0f * (Pi / 2.0f) : (Pi / 2.0f))
                    :
                    3.0f * (Pi / 2.0f);
    
            const float phi = asinf(cartesian.y / cartesian.norm()) + (Pi / 2.0f);

            // Reading the pixel of the image corresponding to the spherical coordinates of the pixel in world space
            const int index_src = 3 * (((int) (phi * param.img_scale_y)) * param.img_width + (int) (theta * param.img_scale_x));
            

            // Copying its color onto the screen
            // const int index = 3 * (jwidth + i);
            // texture_pixels[index]     = orig_pixels[index_src];
            // texture_pixels[index + 1] = orig_pixels[index_src + 1];
            // texture_pixels[index + 2] = orig_pixels[index_src + 2];
            std::memcpy(param.texture_pixels + sl_param.index, param.orig_pixels + index_src, 3);
            sl_param.index += 3;
        }
#else
        /*
            cartesian.x == 0 iff
                scaled_x_axis.x * (i - half_scr_width) + pre_cartesian.x == 0
                i == half_scr_width - pre_cartesian.x / scaled_x_axis.x
                if (-pre_cartesian.x / scaled_x_axis.x) is an integer
        */


        const float lim = half_scr_width + ((param.scaled_x_axis.x != 0) ? -pre_cartesian.x / param.scaled_x_axis.x : infinity);
        const bool lim_pos = lim >= 0.0f;
        const bool two_loops_needed = lim_pos && (lim < width);
        const bool need_limit_case = two_loops_needed && (lim - nearbyintf(lim) < 1e-5f);
        const float init_cartx = fma(param.scaled_x_axis.x, -half_scr_width, pre_cartesian.x);
        const bool starts_pos = init_cartx >= 0.0f;
        const float const_before = starts_pos ? 3.0f * (Pi / 2.0f) : (Pi / 2.0f);
        [[maybe_unused]] const float const_after = (2.0f * Pi) - const_before;
        sl_param.lim_int = static_cast<int>(lim);
        const int last_first_loop = !lim_pos ?
              width
            : std::max(0, std::min(sl_param.lim_int + (!need_limit_case), width));
        
        sl_param.cartesian = fma(param.scaled_x_axis, (-1) - half_scr_width, pre_cartesian);
        
        sl_param.x = (sl_param.cartesian.x != 0) ?
              sl_param.cartesian.z / sl_param.cartesian.x
            : (sl_param.cartesian.z + 0.1f * param.scaled_x_axis.z) / (sl_param.cartesian.x + 0.1f * param.scaled_x_axis.x);
        sl_param.theta = (sl_param.cartesian.x != 0) ? (atanf(sl_param.x) + const_before) : (starts_pos ? 0 : Pi);
        sl_param.y = sl_param.cartesian.y / sqrt(sl_param.cartesian.x * sl_param.cartesian.x + sl_param.cartesian.z * sl_param.cartesian.z);
        sl_param.phi = atanf(sl_param.y) + (Pi / 2.0f);

#define POLYNOMIAL_SOLVE
#ifdef POLYNOMIAL_SOLVE

        int k1_theta, k2_theta;
        compute_bounds_theta_update(k1_theta, k2_theta, sl_param.cartesian, param.scaled_x_axis);

        int k1_phi, k2_phi;
        [[maybe_unused]] phi_test_bounds ptb = compute_bounds_phi_update(k1_phi, k2_phi, sl_param.cartesian, param.scaled_x_axis);

        using enum loop_version::theta;
        using enum loop_version::phi;
        using enum loop_version::limit;
        using enum phi_test_bounds;

        if (!(k2_theta < 0 || k1_theta >= width)) {
        // if ((k1_theta >= 0 && k1_theta < width) || (k2_theta >= 0 && k2_theta < width)) {
            //const int index_start = index;
            const int b1 = std::min(k1_theta, width);
            const int b3 = std::min(k2_theta, width);
            const int b2 = std::min(b3, sl_param.lim_int + (!need_limit_case));

//#define TEST_SPLIT
#ifdef TEST_SPLIT
            int i;
            for (i = 0; i < b1; i++) {
                // deriv
                // Red
                param.texture_pixels[sl_param.index]     = 0;
                param.texture_pixels[sl_param.index + 1] = 0;
                param.texture_pixels[sl_param.index + 2] = '\255';
                sl_param.index += 3;
            }
            for (; i < b2; i++) {
                // tan (before)
                // Blue
                param.texture_pixels[sl_param.index]     = '\255';
                param.texture_pixels[sl_param.index + 1] = 0;
                param.texture_pixels[sl_param.index + 2] = 0;
                sl_param.index += 3;
            }
            if (need_limit_case) {
                // White
                param.texture_pixels[sl_param.index]     = '\255';
                param.texture_pixels[sl_param.index + 1] = '\255';
                param.texture_pixels[sl_param.index + 2] = '\255';
                sl_param.index += 3;
                i++;
            }
            for (; i < b3; i++) {
                // tan (after)
                // Green
                param.texture_pixels[sl_param.index]     = 0;
                param.texture_pixels[sl_param.index + 1] = '\255';
                param.texture_pixels[sl_param.index + 2] = 0;
                sl_param.index += 3;
            }
            for (; i < width; i++) {
                // deriv
                // Yellow
                param.texture_pixels[sl_param.index]     = 0;
                param.texture_pixels[sl_param.index + 1] = '\255';
                param.texture_pixels[sl_param.index + 2] = '\255';
                sl_param.index += 3;
            }
            continue;
#else
#define TEMPLATE_LOOPS
#ifdef TEMPLATE_LOOPS
            //print_ptb(ptb);

            ptb = (ptb == NeverTest) || ((!(ptb == AlwaysTest)) && (((ptb == TestOutside) && (k1_phi < 0 && k2_phi >= width)) || ((ptb == TestBetween) && (k2_phi < 0 || k1_phi >= width)))) ?
                NeverTest : AlwaysTest;
            
            switch (ptb) {
                case NeverTest: {
                    sl_param.index_loop = 0;
                    segment_loop<UpdateThetaDerivative, NoTestPhi, NotLimitCase>(param, sl_param, b1, 0.0f);
                    segment_loop<UpdateThetaAtanBefore, NoTestPhi, NotLimitCase>(param, sl_param, b2, const_before);
                    if (need_limit_case) {
                        segment_loop<UpdateThetaAtanBefore, NoTestPhi, LimitCase>(param, sl_param, sl_param.lim_int + 1, 0.0f);
                    }
                    const float const_correct = two_loops_needed ? const_after : const_before;
                    segment_loop<UpdateThetaAtanAfter, NoTestPhi, NotLimitCase> (param, sl_param, b3,    const_correct);
                    segment_loop<UpdateThetaTestPhi,   NoTestPhi, NotLimitCase> (param, sl_param, width, const_correct);
                    break;
                }
                case AlwaysTest: {

                    sl_param.index_loop = 0;
                    segment_loop<UpdateThetaDerivative, TestPhi, NotLimitCase>  (param, sl_param, b1, 0.0f);
                    segment_loop<UpdateThetaAtanBefore, TestPhi, NotLimitCase>  (param, sl_param, b2, const_before);
                    if (need_limit_case) {
                        segment_loop<UpdateThetaAtanBefore, TestPhi, LimitCase> (param, sl_param, sl_param.lim_int + 1, 0.0f);
                    }
                    const float const_correct = two_loops_needed ? const_after : const_before;
                    segment_loop<UpdateThetaAtanAfter, TestPhi, NotLimitCase>   (param, sl_param, b3,    const_correct);
                    segment_loop<UpdateThetaTestPhi,   TestPhi, NotLimitCase>   (param, sl_param, width, const_correct);
                    break;
                }
                default: break;
            }
            
#else
            int i;
            for (i = 0; i < b1; i++) {
                // deriv
                sl_param.cartesian += param.scaled_x_axis;
            
                const float nx = sl_param.cartesian.z / sl_param.cartesian.x;
                const float dx = nx - sl_param.x;
                const float mx = (sl_param.x + nx) * 0.5f;
                sl_param.theta += dx / (1 + mx * mx);
                sl_param.x = nx;

                const float ny = sl_param.cartesian.y / sqrt(sl_param.cartesian.x * sl_param.cartesian.x + sl_param.cartesian.z * sl_param.cartesian.z);
                const float dy = ny - sl_param.y;
                const bool needs_reset_phi = std::abs(dy) > tan_reset_threshold;
                const float my = (sl_param.y + ny) * 0.5f;
                sl_param.phi = needs_reset_phi ? (atanf(ny) + (Pi / 2.0f)) : sl_param.phi + (dy / (1 + my * my));
                sl_param.y = ny;

                const int index_src_1 = ((static_cast<int>(sl_param.phi * param.img_scale_y)) * param.img_width + static_cast<int>(sl_param.theta * param.img_scale_x));
                const int index_src_2 = (index_src_1 << 1) + index_src_1;
                const int index_src = std::max(0, index_src_2);
                std::memcpy(param.texture_pixels + sl_param.index, param.orig_pixels + index_src, 3);

                sl_param.index += 3;
            }
            // const int i0 = i;
            for (; i < b2; i++) {
                // tan (before)
                sl_param.cartesian += param.scaled_x_axis;
            
                const float nx = sl_param.cartesian.z / sl_param.cartesian.x;
                sl_param.theta = atanf(nx) + const_before;
                sl_param.x = nx;

                const float ny = sl_param.cartesian.y / sqrt(sl_param.cartesian.x * sl_param.cartesian.x + sl_param.cartesian.z * sl_param.cartesian.z);
                const float dy = ny - sl_param.y;
                const bool needs_reset_phi = std::abs(dy) > tan_reset_threshold;
                const float my = (sl_param.y + ny) * 0.5f;
                sl_param.phi = needs_reset_phi ? (atanf(ny) + (Pi / 2.0f)) : sl_param.phi + (dy / (1 + my * my));
                sl_param.y = ny;

                const int index_src_1 = ((static_cast<int>(sl_param.phi * param.img_scale_y)) * param.img_width + static_cast<int>(sl_param.theta * param.img_scale_x));
                const int index_src_2 = (index_src_1 << 1) + index_src_1;
                const int index_src = std::max(0, index_src_2);
                std::memcpy(param.texture_pixels + sl_param.index, param.orig_pixels + index_src, 3);

                // if (i == i0 || i == b2 - 1) {
                //     texture_pixels[index]     = 0;
                //     texture_pixels[index + 1] = '\255';
                //     texture_pixels[index + 2] = '\255';
                // }

                sl_param.index += 3;
            }
            if (need_limit_case) {
                
                sl_param.cartesian += param.scaled_x_axis;

                sl_param.theta = (sl_param.theta < Pi / 2) || (sl_param.theta > 3 * Pi /2) ? 0 : Pi;
                
                const float ny = sl_param.cartesian.y / sqrt(sl_param.cartesian.x * sl_param.cartesian.x + sl_param.cartesian.z * sl_param.cartesian.z);
                const float dy = ny - sl_param.y;
                const bool needs_reset_phi = std::abs(dy) > tan_reset_threshold;
                const float my = (sl_param.y + ny) * 0.5f;
                sl_param.phi = needs_reset_phi ? (atanf(ny) + (Pi / 2.0f)) : sl_param.phi + (dy / (1 + my * my));
                sl_param.y = ny;

                const int index_src_r = 3 * ((static_cast<int>(sl_param.phi * param.img_scale_y)) * param.img_width + static_cast<int>(sl_param.theta * param.img_scale_x));
                const int index_src = std::max(0, index_src_r);
                std::memcpy(param.texture_pixels + sl_param.index, param.orig_pixels + index_src, 3);

                // texture_pixels[index]     = 0;
                // texture_pixels[index + 1] = 0;
                // texture_pixels[index + 2] = '\255';

                sl_param.index += 3;
                i = sl_param.lim_int + 1;
            }
            //const bool blue = i == 0;
            //if (j == 0) printf("theta %f\n", theta);
            const float const_correct = (!two_loops_needed) ? const_before : const_after;
            //const int i0 = i;
            for (; i < b3; i++) {
                // tan (after)
                sl_param.cartesian += param.scaled_x_axis;
            
                sl_param.x = sl_param.cartesian.z / sl_param.cartesian.x;
                sl_param.theta = atanf(sl_param.x) + const_correct;//(i==i0 && !need_limit_case ? const_before : const_correct); // UGLY, but correct. Proper fix to be found

                const float ny = sl_param.cartesian.y / sqrt(sl_param.cartesian.x * sl_param.cartesian.x + sl_param.cartesian.z * sl_param.cartesian.z);
                const float dy = ny - sl_param.y;
                const bool needs_reset_phi = std::abs(dy) > tan_reset_threshold;
                const float my = (sl_param.y + ny) * 0.5f;
                sl_param.phi = needs_reset_phi ? (atanf(ny) + (Pi / 2.0f)) : sl_param.phi + (dy / (1 + my * my));
                sl_param.y = ny;

                const int index_src_1 = ((static_cast<int>(sl_param.phi * param.img_scale_y)) * param.img_width + static_cast<int>(sl_param.theta * param.img_scale_x));
                const int index_src_2 = (index_src_1 << 1) + index_src_1;
                const int index_src = std::max(0, index_src_2);
                std::memcpy(param.texture_pixels + sl_param.index, param.orig_pixels + index_src, 3);

                // if (i == b2 || i == b2 + 1 || i == b3 - 1) {
                //     texture_pixels[index]     = 0;
                //     texture_pixels[index + 1] = 0;
                //     texture_pixels[index + 2] = '\255';
                // }
                // if (blue) {
                //     texture_pixels[index] = '\255';
                // }

                sl_param.index += 3;
            }
            for (; i < width; i++) {
                // deriv
                sl_param.cartesian += param.scaled_x_axis;

                const float ny = sl_param.cartesian.y / sqrt(sl_param.cartesian.x * sl_param.cartesian.x + sl_param.cartesian.z * sl_param.cartesian.z);
                const float dy = ny - sl_param.y;
                const bool needs_reset_phi = std::abs(dy) > tan_reset_threshold;
                const float my = (sl_param.y + ny) * 0.5f;
                sl_param.phi = needs_reset_phi ? (atanf(ny) + (Pi / 2.0f)) : sl_param.phi + (dy / (1 + my * my));
                sl_param.y = ny;

                const float nx = sl_param.cartesian.z / sl_param.cartesian.x;
                const float dx = nx - sl_param.x;
                const float mx = (sl_param.x + nx) * 0.5f;
                sl_param.theta = needs_reset_phi ? (atanf(nx) + const_correct) : sl_param.theta + (dx / (1 + mx * mx));
                sl_param.x = nx;

                const int index_src_1 = ((static_cast<int>(sl_param.phi * param.img_scale_y)) * param.img_width + static_cast<int>(sl_param.theta * param.img_scale_x));
                const int index_src_2 = (index_src_1 << 1) + index_src_1;
                const int index_src = std::max(0, index_src_2);
                std::memcpy(param.texture_pixels + sl_param.index, param.orig_pixels + index_src, 3);

                sl_param.index += 3;
            }
#endif
#endif
        }
        else {
            // one segment
#ifdef TEMPLATE_LOOPS
            ptb =  (ptb == NeverTest)
                || ((!(ptb == AlwaysTest))
                        &&    (((ptb == TestOutside) && (k1_phi < 0 && k2_phi >= width))
                            || ((ptb == TestBetween) && (k2_phi < 0 || k1_phi >= width)))) ?
                NeverTest : AlwaysTest;
            sl_param.index_loop = 0;
            switch (ptb) {
                case NeverTest:
                    segment_loop<UpdateThetaDerivative, NoTestPhi, NotLimitCase>(param, sl_param, last_first_loop, 0.0f);
                    break;
                default:
                    segment_loop<UpdateThetaDerivative, TestPhi,   NotLimitCase>(param, sl_param, last_first_loop, 0.0f);
                    break;
            }
#else
            for (int i = 0; i < last_first_loop; i++) {
                sl_param.cartesian += param.scaled_x_axis;
                
                const float nx = sl_param.cartesian.z / sl_param.cartesian.x;
                const float dx = nx - sl_param.x;
                const float mx = (sl_param.x + nx) * 0.5f;
                sl_param.theta += dx / (1 + mx * mx);
                sl_param.x = nx;

                const float ny = sl_param.cartesian.y / sqrt(sl_param.cartesian.x * sl_param.cartesian.x + sl_param.cartesian.z * sl_param.cartesian.z);
                const float dy = ny - sl_param.y;
                const bool needs_reset_phi = std::abs(dy) > tan_reset_threshold;
                const float my = (sl_param.y + ny) * 0.5f;
                sl_param.phi = needs_reset_phi ? (atanf(ny) + (Pi / 2.0f)) : sl_param.phi + (dy / (1 + my * my));
                sl_param.y = ny;

                const int index_src_1 = ((static_cast<int>(sl_param.phi * param.img_scale_y)) * param.img_width + static_cast<int>(sl_param.theta * param.img_scale_x));
                const int index_src_2 = (index_src_1 << 1) + index_src_1;
                const int index_src = std::max(0, index_src_2);
                std::memcpy(param.texture_pixels + sl_param.index, param.orig_pixels + index_src, 3);

                // if (starts_pos) {
                //     texture_pixels[index] = '\255';
                // }
                // texture_pixels[index + 2] = '\255';
                
#define DRAW_RESET
#ifdef DRAW_RESET
                constexpr bool needs_reset_theta = false; // I don't know where this needs_reset_theta below comes from
                if (needs_reset_theta || needs_reset_phi) {
                    param.texture_pixels[sl_param.index]     = 255 * needs_reset_theta;
                    param.texture_pixels[sl_param.index + 1] = 255 * (needs_reset_theta && needs_reset_phi);
                    param.texture_pixels[sl_param.index + 2] = 255 * needs_reset_phi;
                }
#endif
#define SHOULD_BE_POLY
#ifdef SHOULD_BE_POLY
                // if (needs_reset_theta) {
                //     texture_pixels[index]     = 0;
                //     texture_pixels[index + 1] = '\128';
                //     texture_pixels[index + 2] = '\255';
                // }
#endif

                sl_param.index += 3;
            }
#endif
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
            sl_param.cartesian += param.scaled_x_axis;
            
            //const float theta = atanf(cartesian.z / cartesian.x) + const_before;
            const float nx = sl_param.cartesian.z / sl_param.cartesian.x;
            const float dx = nx - sl_param.x;
            const bool needs_reset_theta = std::abs(dx) > tan_reset_threshold;
            const float mx = (sl_param.x + nx) * 0.5f;
            sl_param.theta = needs_reset_theta ? (atanf(nx) + const_before) : sl_param.theta + (dx / (1 + mx * mx));
            //if (needs_reset_theta) atan_cpt++;
            sl_param.x = nx;

            //  float phi = asinf(cartesian.y / cartesian.norm()) + (Pi / 2.0f);
            //const float phi = atanf(cartesian.y / sqrt(cartesian.x * cartesian.x + cartesian.z * cartesian.z)) + (Pi / 2.0f);
            //if (cartesian.x * cartesian.x + cartesian.z * cartesian.z == 0) printf("First loop: div by 0\n");
            const float ny = sl_param.cartesian.y / sqrt(sl_param.cartesian.x * sl_param.cartesian.x + sl_param.cartesian.z * sl_param.cartesian.z);
            const float dy = ny - sl_param.y;
            const bool needs_reset_phi = std::abs(dy) > tan_reset_threshold;
            const float my = (sl_param.y + ny) * 0.5f;
            sl_param.phi = needs_reset_phi ? (atanf(ny) + (Pi / 2.0f)) : sl_param.phi + (dy / (1 + my * my));
            //if (needs_reset_phi) atan_cpt++;
            sl_param.y = ny;

            const int index_src_1 = ((static_cast<int>(sl_param.phi * param.img_scale_y)) * param.img_width + static_cast<int>(sl_param.theta * param.img_scale_x));
            const int index_src_2 = (index_src_1 << 1) + index_src_1;
            // if (index_src_r < 0) printf("FIRST LOOP index_src < 0\n");
            // if (index_src_r > max_index_src) printf("FIRST LOOP index_src >= max\n");
            //const int index_src = std::clamp(index_src_r, 0, max_index_src);
            const int index_src = std::max(0, index_src_2);
#ifndef BUFFER_CPY
            std::memcpy(param.texture_pixels + sl_param.index, param.orig_pixels + index_src, 3);
#else
            if (index_src != prev_index_src + 3) {
                // if (cpt != 3) {
                //     for (int k = 0; k < cpt; k += 3) {
                //         texture_pixels[init_index + k]     = 0;
                //         texture_pixels[init_index + k + 1] = 0;
                //         texture_pixels[init_index + k + 2] = '\255';
                //     }
                // }
                // else {
                    std::memcpy(param.texture_pixels + init_index, param.orig_pixels + init_index_src, cpt);
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
                param.texture_pixels[index]     = 255 * needs_reset_theta;
                param.texture_pixels[index + 1] = 255 * (needs_reset_theta && needs_reset_phi);
                param.texture_pixels[index + 2] = 255 * needs_reset_phi;
            }
#endif
//#define SHOULD_BE_POLY
#ifdef SHOULD_BE_POLY
            if (needs_reset_theta) {
                param.texture_pixels[index]     = 0;
                param.texture_pixels[index + 1] = '\128';
                param.texture_pixels[index + 2] = '\255';
            }
#endif

            sl_param.index += 3;
        }
        if (two_loops_needed) {
            if (need_limit_case) {
                sl_param.cartesian += param.scaled_x_axis;

                sl_param.theta = (sl_param.theta < Pi / 2) || (sl_param.theta > 3 * Pi / 2) ? 0 : Pi;
                
                //const float phi = asinf(cartesian.y / sqrt(cartesian.y * cartesian.y + cartesian.z * cartesian.z)) + (Pi / 2.0f);
                //const float phi = atanf(cartesian.y / sqrt(cartesian.x * cartesian.x + cartesian.z * cartesian.z)) + (Pi / 2.0f);
                //if (cartesian.x * cartesian.x + cartesian.z * cartesian.z == 0) printf("Limit case: div by 0\n");
                const float ny = sl_param.cartesian.y / sqrt(sl_param.cartesian.x * sl_param.cartesian.x + sl_param.cartesian.z * sl_param.cartesian.z);
                const float dy = ny - sl_param.y;
                const bool needs_reset_phi = std::abs(dy) > tan_reset_threshold;
                const float my = (sl_param.y + ny) * 0.5f;
                sl_param.phi = needs_reset_phi ? (atanf(ny) + (Pi / 2.0f)) : sl_param.phi + (dy / (1 + my * my));
                //if (needs_reset_phi) atan_cpt++;
                sl_param.y = ny;

                const int index_src_r = 3 * ((static_cast<int>(sl_param.phi * param.img_scale_y)) * param.img_width + static_cast<int>(sl_param.theta * param.img_scale_x));
                // if (index_src_r < 0) printf("MID index_src < 0\n");
                // if (index_src_r > max_index_src) printf("MID index_src >= max\n");
                //const int index_src = std::clamp(index_src_r, 0, max_index_src);
                const int index_src = std::max(0, index_src_r);
#ifndef BUFFER_CPY
                std::memcpy(param.texture_pixels + sl_param.index, param.orig_pixels + index_src, 3);
#else
                if (index_src != prev_index_src + 3) {
                    // if (cpt != 3) {
                    //     for (int k = 0; k < cpt; k += 3) {
                    //         texture_pixels[init_index + k]     = 0;
                    //         texture_pixels[init_index + k + 1] = 0;
                    //         texture_pixels[init_index + k + 2] = '\255';
                    //     }
                    // }
                    // else {
                        std::memcpy(param.texture_pixels + init_index, param.orig_pixels + init_index_src, cpt);
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
                    param.texture_pixels[index]     = 0;
                    param.texture_pixels[index + 1] = 0;
                    param.texture_pixels[index + 2] = '\255';
                }
#endif
#ifdef SHOULD_BE_POLY
                param.texture_pixels[index]     = 0;
                param.texture_pixels[index + 1] = '\128';
                param.texture_pixels[index + 2] = '\255';
#endif

                sl_param.index += 3;
                i = sl_param.lim_int + 1;
            }
            
            sl_param.x = infinity;
            for (; i < width; i++) {
                sl_param.cartesian += param.scaled_x_axis;

                //const float phi = asinf(cartesian.y / cartesian.norm()) + (Pi / 2.0f);
                //const float phi = atanf(cartesian.y / sqrt(cartesian.x * cartesian.x + cartesian.z * cartesian.z)) + (Pi / 2.0f);
                //if (cartesian.x * cartesian.x + cartesian.z * cartesian.z == 0) printf("Second loop: div by 0\n");
                const float ny = sl_param.cartesian.y / sqrt(sl_param.cartesian.x * sl_param.cartesian.x + sl_param.cartesian.z * sl_param.cartesian.z);
                const float dy = ny - sl_param.y;
                const bool needs_reset_phi = std::abs(dy) > tan_reset_threshold;
                const float my = (sl_param.y + ny) * 0.5f;
                sl_param.phi = needs_reset_phi ? (atanf(ny) + (Pi / 2.0f)) : sl_param.phi + (dy / (1 + my * my));
                //if (needs_reset_phi) atan_cpt++;
                sl_param.y = ny;

                // const float theta = atanf(cartesian.z / cartesian.x) + const_after;
                // atan_cpt++;
                const float nx = sl_param.cartesian.z / sl_param.cartesian.x;
                const float dx = nx - sl_param.x;
                const bool needs_reset_theta = std::abs(dx) > tan_reset_threshold;
                const float mx = (sl_param.x + nx) * 0.5f;
                sl_param.theta = needs_reset_theta || needs_reset_phi ? (atanf(nx) + const_after) : sl_param.theta + (dx / (1 + mx * mx));
                //if (needs_reset_theta) atan_cpt++;
                sl_param.x = nx;

                const int index_src_1 = ((static_cast<int>(sl_param.phi * param.img_scale_y)) * param.img_width + static_cast<int>(sl_param.theta * param.img_scale_x));
                const int index_src_2 = (index_src_1 << 1) + index_src_1;
                // if (index_src_r < 0) printf("SECOND LOOP index_src < 0, theta %f, phi %f, theta * img_scale_x %f, phi * img_scale_y %f, index_src %d\n",
                //     theta, phi, theta * img_scale_x, phi * img_scale_y, index_src_r);
                // if (index_src_r > max_index_src) printf("SECOND LOOP index_src >= max\n");
                //const int index_src = std::clamp(index_src_r, 0, max_index_src);
                const int index_src = std::max(0, index_src_2);
#ifndef BUFFER_CPY
                std::memcpy(param.texture_pixels + sl_param.index, param.orig_pixels + index_src, 3);
#else
                if (index_src != prev_index_src + 3) {
                    // if (cpt != 3) {
                    //     for (int k = 0; k < cpt; k += 3) {
                    //         texture_pixels[init_index + k]     = 0;
                    //         texture_pixels[init_index + k + 1] = 0;
                    //         texture_pixels[init_index + k + 2] = '\255';
                    //     }
                    // }
                    // else {
                        std::memcpy(param.texture_pixels + init_index, param.orig_pixels + init_index_src, cpt);
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
                    param.texture_pixels[index]     = 255 * needs_reset_theta;
                    param.texture_pixels[index + 1] = 255 * (needs_reset_theta && needs_reset_phi);
                    param.texture_pixels[index + 2] = 255 * needs_reset_phi;
                }
#endif
#ifdef SHOULD_BE_POLY
                if (needs_reset_theta) {
                    texture_pixels[index]     = 0;
                    texture_pixels[index + 1] = '\128';
                    texture_pixels[index + 2] = '\255';
                }
#endif

                sl_param.index += 3;
            }

#ifdef BUFFER_CPY
            
            std::memcpy(param.texture_pixels + init_index, param.orig_pixels + init_index_src, cpt);
#endif
        }
#endif
#endif
    }
    SDL_UnlockTexture(param.txt);
    SDL_RenderClear(param.scr.renderer);
    SDL_RenderCopy(param.scr.renderer, param.txt, &param.scr.srcrect, &param.scr.dstrect);
    SDL_RenderPresent(param.scr.renderer);
}


// BUFFER_CPY: bad idea, goes from 26fps to 20fps...
// POLYNOMIAL_SOLVE: same