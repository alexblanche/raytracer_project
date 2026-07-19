#include "other/sky/sky_render.hpp"

#include "parallel/parallel.hpp"

#include <cstring>
#include <iostream>

static constexpr bool RENDER_PARALLEL = true;
static constexpr int nb_threads = 8;

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
    }

    k1_io = k1;
    k2_io = k2;
}

enum class phi_test_bounds {
    AlwaysTest, NeverTest, TestBetween, TestOutside
};

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

void run_render_loop(int j_start, int j_end, const render_parameters& param) {

    for (int j = j_start; j < j_end; j++) {

        segment_loop_parameters sl_param;
        sl_param.index = j * param.texture_pitch;

        // Pre-computation of the cartesian coordinates of the pixel in world space
        const sky::vector pre_cartesian = fma(param.scaled_y_axis, j - half_scr_height, param.axes_center);
        
        const float lim = half_scr_width + ((param.scaled_x_axis.x != 0) ? -pre_cartesian.x / param.scaled_x_axis.x : infinity);
        const bool lim_pos = lim >= 0.0f;
        const bool two_loops_needed = lim_pos && (lim < width);
        const bool need_limit_case = two_loops_needed && (lim - nearbyintf(lim) < 1e-5f);
        const float init_cartx = fma(param.scaled_x_axis.x, -half_scr_width, pre_cartesian.x);
        const bool starts_pos = init_cartx >= 0.0f;
        const float const_before = starts_pos ? 3.0f * (Pi / 2.0f) : (Pi / 2.0f);
        const float const_after = (2.0f * Pi) - const_before;
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


        int k1_theta, k2_theta;
        compute_bounds_theta_update(k1_theta, k2_theta, sl_param.cartesian, param.scaled_x_axis);

        int k1_phi, k2_phi;
        [[maybe_unused]] phi_test_bounds ptb = compute_bounds_phi_update(k1_phi, k2_phi, sl_param.cartesian, param.scaled_x_axis);

        using enum loop_version::theta;
        using enum loop_version::phi;
        using enum loop_version::limit;
        using enum phi_test_bounds;

        if (!(k2_theta < 0 || k1_theta >= width)) {
            
            const int b1 = std::min(k1_theta, width);
            const int b3 = std::min(k2_theta, width);
            const int b2 = std::min(b3, sl_param.lim_int + (!need_limit_case));

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
        }
        else {
            // one segment
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
        }
    }
}

void render(const render_parameters& param) {

    SDL_LockTexture(param.txt, nullptr, reinterpret_cast<void**>(&param.texture_pixels), &param.texture_pitch);

    if constexpr (RENDER_PARALLEL) {
        parallel_for(height, [&] (int j_start, int j_end) {
            run_render_loop(j_start, j_end, param);
        },
        nb_threads);
    }
    else {
        run_render_loop(0, height, param);
    }

    SDL_UnlockTexture(param.txt);
    SDL_RenderClear(param.scr.renderer);
    SDL_RenderCopy(param.scr.renderer, param.txt, &param.scr.srcrect, &param.scr.dstrect);
    SDL_RenderPresent(param.scr.renderer);
}
