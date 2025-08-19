#pragma once

#include <vector>
#include "screen/color.hpp"
#include "scene/scene.hpp"

/* Sequential loop */
void render_loop_seq(std::vector<vector<rt::color>>& matrix,
    scene& scene, const unsigned int number_of_bounces, const bool russian_roulette);

/* Main render loop */
void render_loop_parallel(std::vector<std::vector<rt::color>>& matrix,
    scene& scene, const unsigned int number_of_bounces, const bool russian_roulette, const unsigned int iteration);

/* Render loop that handles time measurement
   If time_all is true, all lines produce a time measurement and output the estimated total time.
   It time_all is false, only the total time is output at the end.
 */
void render_loop_parallel_time(std::vector<std::vector<rt::color>>& matrix,
    scene& scene, const unsigned int number_of_bounces, const bool time_all);


void render_loop_parallel_multisample(std::vector<std::vector<rt::color>>& matrix,
    scene& scene, const unsigned int number_of_bounces, const unsigned int number_of_samples);