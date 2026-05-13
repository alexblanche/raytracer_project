#pragma once

#include <vector>
#include "screen/color.hpp"
#include "scene/scene.hpp"
#include "main_menu/runtime_parameters.hpp"

/* Sequential loop */
void render_loop_seq(std::vector<vector<rt::color>>& matrix,
    const scene& scene, unsigned int number_of_bounces, russian_roulette_mode russian_roulette);

/* Main render loop */
void render_loop_parallel(std::vector<std::vector<rt::color>>& matrix,
    const scene& scene, unsigned int number_of_bounces, russian_roulette_mode russian_roulette, unsigned int iteration);

/* Render loop that handles time measurement */
void render_loop_parallel_time(std::vector<std::vector<rt::color>>& matrix,
    const scene& scene, unsigned int number_of_bounces, time_mode time_mode);


void render_loop_parallel_multisample(std::vector<std::vector<rt::color>>& matrix,
    const scene& scene, unsigned int number_of_bounces, unsigned int number_of_samples);