#include <string>
#include <iostream>
#include <signal.h>
#include <SDL2/SDL.h>
#include <cmath>

#include "screen/screen.hpp"
#include "light/hit.hpp"

namespace rt {

	/**
	 * The screen class inherites from the image class in order
	 * to draw something on the screen. It also wraps the SDL
	 * initialization calls. Only one screen should be created.
	 */

	// Indicates how many instances of screen exist.
	int screen::initialized = 0;

	static void sigint_handler(int) {
		exit(0);
	}

	/**
	 * Main constructor, uses width and height.
	 */
	screen::screen(int width, int height)
		
		: image(width, height) {

		if(initialized == 0) {
			if(SDL_Init( SDL_INIT_VIDEO ) == -1) {
				std::cerr << "Cannot initialize SDL : "
					<< SDL_GetError() << std::endl;
				exit(-1);
			}
			signal(SIGINT, sigint_handler);
		}
		initialized += 1;
	}

	/**
	 * Destructor. Decrements the initialized counter.
	 */
	screen::~screen() {
		initialized--;
	}

	/**
	 * Flushes the buffer to the screen
	 */
	void screen::update() const {
		SDL_RenderPresent(renderer);
	}

	/**
	 * Same as update, but first copies the texture onto the renderer
	 */
	void screen::update_from_texture() const {
		SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, &srcrect, &dstrect);
        SDL_RenderPresent(renderer);
	}


	/****************************************************************************************************/
	/** Event processing **/

	/* Wait indefinitely for the next quit event */
	screen::quit_event screen::wait_quit_event() const {
		SDL_Event event;
		while(SDL_WaitEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT:
					return screen::quit_event::QuitEvent;
				case SDL_KEYDOWN:
					return screen::quit_event::KeyBoardEvent;
				default:
					break;
			}
		}
		return screen::quit_event::Error;
	}

	/* Stop at the next quit event */
	screen::quit_event screen::is_quit_event() const {
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT:
					return screen::quit_event::QuitEvent;
				case SDL_KEYDOWN:
					return screen::quit_event::KeyBoardEvent;
				default:
					break;
			}
		}
		return screen::quit_event::Error;
	}

	/* Wait indefinitely for the next keyboard or quit event */	
	screen::key screen::wait_keyboard_event() const {
		SDL_Event event;
		
		while(SDL_WaitEvent(&event)) {
			
			switch(event.type) {
				
				case SDL_QUIT:
					return screen::key::QuitEvent;
				
				case SDL_KEYDOWN:
					switch(event.key.keysym.scancode) {
						
						case SDL_SCANCODE_ESCAPE:
							return screen::key::QuitEvent;
						
						case SDL_SCANCODE_SPACE:
						case SDL_SCANCODE_RETURN:
						case SDL_SCANCODE_KP_ENTER:
							return screen::key::SpaceEnter;

						case SDL_SCANCODE_B:
							return screen::key::B;
						
						case SDL_SCANCODE_R:
							return screen::key::R;
						
						default:
							break;
					}
					break;
				
				case SDL_MOUSEBUTTONDOWN:
					printf("\nX = %d, Y = %d", event.button.x, event.button.y);
					break;

				default:
					break;
			}
		}
		return screen::key::Other;
	}

	/* Same as wait_keyboard_event, with poll events */
	screen::key screen::poll_keyboard_event() const {
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT:
					return screen::key::QuitEvent;
				case SDL_KEYDOWN:
					switch(event.key.keysym.scancode) {
						
						case SDL_SCANCODE_ESCAPE:
							return screen::key::QuitEvent;
						
						case SDL_SCANCODE_SPACE:
						case SDL_SCANCODE_RETURN:
						case SDL_SCANCODE_KP_ENTER:
							return screen::key::SpaceEnter;
						
						case SDL_SCANCODE_B:
							return screen::key::B;
						
						case SDL_SCANCODE_R:
							return screen::key::R;
						
						default:
							break;
					}
					break;
				// case SDL_MOUSEBUTTONDOWN:
				// 	printf("\nX = %d, Y = %d", event.button.x, event.button.y);
				// 	break;
				default:
					break;
			}
		}
		return screen::key::Other;
	}

	/****************************************************************************************************/

	/**
	 * Copies the rt::color matrix onto the screen, by averaging the number_of_rays colors per pixel
	 */
	void screen::fast_copy(std::vector<std::vector<color>>& matrix,
		const size_t width, const size_t height,
		const unsigned int number_of_rays) const {

		const real invN = 1.0f / static_cast<real>(number_of_rays);

		char* texture_pixels;
		int texture_pitch;

		SDL_LockTexture(texture, NULL, (void**) &texture_pixels, &texture_pitch);
		
		const unsigned int padding = texture_pitch % 3;
		const unsigned int shift = 3 * width + padding;

		for (size_t i = 0; i < width; i++) {
			const std::vector<color>& line = matrix[i];
			const unsigned int threei = 3 * i;

            for (size_t j = 0; j < height; j++) {

				const color& pixel_col = line[j];
				color avg = pixel_col * invN;
				avg.in_place_max_out();
				const unsigned int index = j * shift + threei;
				texture_pixels[index] 	  = static_cast<Uint8>(avg.get_red());
				texture_pixels[index + 1] = static_cast<Uint8>(avg.get_green());
				texture_pixels[index + 2] = static_cast<Uint8>(avg.get_blue());
            }
        }
		
        SDL_UnlockTexture(texture);
	}

	/* Copy matrix to the screen with gamma correction */
	void screen::fast_copy_gamma(std::vector<std::vector<color>>& matrix,
		const size_t width, const size_t height,
		const unsigned int number_of_rays, const real gamma) const {

		const real invN = 1.0 / number_of_rays;
		constexpr real inv255 = 1.0 / 255.0;
		const real inv = inv255 * invN;

		char* texture_pixels;
		int texture_pitch;

		SDL_LockTexture(texture, NULL, (void**) &texture_pixels, &texture_pitch);
		const unsigned int padding = texture_pitch % 3;
		const unsigned int shift = 3 * width + padding;
		
		for (size_t i = 0; i < width; i++) {
			const std::vector<color>& line = matrix[i];
			const unsigned int threei = 3 * i;

            for (size_t j = 0; j < height; j++) {
				const color& pixel_col = line[j];
				color corrected = pixel_col * inv;
				corrected ^= gamma;
				corrected *= static_cast<real>(255.0f);
				corrected.in_place_max_out();
				const unsigned int index = j * shift + threei;
				texture_pixels[index] 	  = static_cast<Uint8>(corrected.get_red());
				texture_pixels[index + 1] = static_cast<Uint8>(corrected.get_green());
				texture_pixels[index + 2] = static_cast<Uint8>(corrected.get_blue());
            }
        }
		
        SDL_UnlockTexture(texture);
	}

	/* Copy matrix to the screen with gamma correction and extended Reinhardt local tone mapping */
	void screen::fast_copy_reinhardt(std::vector<std::vector<color>>& matrix,
		const size_t width, const size_t height,
		const unsigned int number_of_rays, const real gamma) const {

		const real invN = 1.0 / number_of_rays;

		// Computation of the maximum luminance
		float max_luminance = 0.0f;
		for (unsigned int i = 0; i < width; i++) {
			const std::vector<color>& line = matrix[i];
			for (unsigned int j = 0; j < height; j++) {
				const rt::color& col = line[j];
				const float luminance = (0.2126 * col.get_red() + 0.7152 * col.get_green() + 0.0722 * col.get_blue()) * invN;
				// const float luminance = (col.get_red() + col.get_green() + col.get_blue()) * invN * 0.333;
				if (luminance > max_luminance)
					max_luminance = luminance;
			}
		}
		const float lwhitecorr = 1.0f / (max_luminance * max_luminance);

		char* texture_pixels;
		int texture_pitch;

		SDL_LockTexture(texture, NULL, (void**) &texture_pixels, &texture_pitch);
		const unsigned int padding = texture_pitch % 3;
        
		//unsigned int index = 0;
		constexpr real inv255 = 1.0f / 255.0f;
		const real inv = inv255 * invN;

		const unsigned int shift = 3 * width + padding;
		for (size_t i = 0; i < width; i++) {
			const std::vector<color>& line = matrix[i];
			const unsigned int threei = 3 * i;
            for (size_t j = 0; j < height; j++) {
				const color& col = line[j];
				const real lr = col.get_red();
				const real lg = col.get_green();
				const real lb = col.get_blue();
				
				const real lin = (0.2126 * lr + 0.7152 * lg + 0.0722 * lb) * inv;
				// const real lin = (lr + lg + lb) * inv * 0.333;
				const real lcorr = (1.0f + lin * lwhitecorr) / (1.0f + lin);

				color corrected = col * (lcorr * inv);
				corrected ^= gamma;
				corrected *= static_cast<real>(255.0f);
				corrected.in_place_max_out();

				const unsigned int index = j * shift + threei;
				texture_pixels[index] 	  = static_cast<Uint8>(corrected.get_red());
				texture_pixels[index + 1] = static_cast<Uint8>(corrected.get_green());
				texture_pixels[index + 2] = static_cast<Uint8>(corrected.get_blue());
            }
        }
		
        SDL_UnlockTexture(texture);
	}
}
