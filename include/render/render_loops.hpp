#pragma once

#include "image/image.hpp"
#include "scene/scene.hpp"
#include "main_menu/runtime_parameters.hpp"

/* Sequential loop */
void render_loop_seq(image& image, const scene& scene, unsigned int number_of_bounces, russian_roulette_mode russian_roulette);

/* Main render loop */
void render_loop_parallel(image& image, const scene& scene, unsigned int number_of_bounces, russian_roulette_mode russian_roulette);

/* Render loop that handles time measurement */
void render_loop_parallel_time(image& image, const scene& scene, unsigned int number_of_bounces, time_mode time_mode);


void render_loop_parallel_multisample(image& image, const scene& scene, unsigned int number_of_bounces, unsigned int number_of_samples);


// Experiment
void render_loop_parallel_all_at_once(image& image, const scene& scene, const unsigned int number_of_bounces,
    const russian_roulette_mode russian_roulette, const unsigned int target);