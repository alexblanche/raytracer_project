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

	/**
	 * @brief Wait indefinitely for the next quit event
	 * @return true if we get a quit event, false if we get a keydown event
	 */
	bool screen::wait_quit_event() const {
		SDL_Event event;
		while(SDL_WaitEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT:
					return true;
					break;
				case SDL_KEYDOWN:
					return false;
					break;
			}
		}
		return false;
	}

	/**
	 * @brief Stop at the next quit event
	 * @return true if we get a quit event, false if we get a keydown event
	 */
	bool screen::is_quit_event() const {
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT:
					return true;
					break;
				case SDL_KEYDOWN:
					return false;
					break;
			}
		}
		return false;
	}

	/**
	 * @brief Wait indefinitely for the next keyboard or quit event
	 * @return
	 * 		1: quit event (Esc or X clicked)
	 * 		2: Space or Enter key
	 * 		3: 'B' key
	 * 		4: 'R' key
	 * 		0: Anything else
	 */
	int screen::wait_keyboard_event() const {
		SDL_Event event;
		while(SDL_WaitEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT:
					return 1;
				case SDL_KEYDOWN:
					switch(event.key.keysym.scancode) {
						case SDL_SCANCODE_ESCAPE:
							return 1;
						case SDL_SCANCODE_SPACE:
						case SDL_SCANCODE_RETURN:
						case SDL_SCANCODE_KP_ENTER:
							return 2;
						case SDL_SCANCODE_B:
							return 3;
						case SDL_SCANCODE_R:
							return 4;
						default:
							break;
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					printf("\nX = %d, Y = %d", event.button.x, event.button.y);
					break;
			}
		}
		return 0;
	}

	/**
	 * Same as wait_keyboard_event, with poll events
	 */
	int screen::poll_keyboard_event() const {
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT:
					return 1;
				case SDL_KEYDOWN:
					switch(event.key.keysym.scancode) {
						case SDL_SCANCODE_ESCAPE:
							return 1;
						case SDL_SCANCODE_SPACE:
						case SDL_SCANCODE_RETURN:
						case SDL_SCANCODE_KP_ENTER:
							return 2;
						case SDL_SCANCODE_B:
							return 3;
						case SDL_SCANCODE_R:
							return 4;
						default:
							break;
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					printf("\nX = %d, Y = %d", event.button.x, event.button.y);
					break;
			}
		}
		return 0;
	}

	/****************************************************************************************************/

	/**
	 * Copies the rt::color matrix onto the screen, by averaging the number_of_rays colors per pixel
	*/
	void screen::copy(std::vector<std::vector<color>>& matrix,
		const size_t width, const size_t height,
		const unsigned int number_of_rays) const {
			
		const real invN = 1.0 / number_of_rays;
		for (size_t i = 0; i < width; i++) {
			for (size_t j = 0; j < height; j++) {
				const color& pixel_col = matrix[i][j];
				// Maxed values
				// const real r = std::min(pixel_col.get_red()   / number_of_rays, (real) 255.0f);
				// const real g = std::min(pixel_col.get_green() / number_of_rays, (real) 255.0f);
				// const real b = std::min(pixel_col.get_blue()  / number_of_rays, (real) 255.0f);
				// set_pixel(i, j, rt::color(r, g, b));

				const Uint8 r = std::min((Uint8) (pixel_col.get_red()   * invN), (Uint8) 255);
				const Uint8 g = std::min((Uint8) (pixel_col.get_green() * invN), (Uint8) 255);
				const Uint8 b = std::min((Uint8) (pixel_col.get_blue()  * invN), (Uint8) 255);
				set_pixel(i, j, r, g, b);
			}
		}
	}

	/**
	 * Same as copy
	 */
	void screen::fast_copy(std::vector<std::vector<color>>& matrix,
		const size_t width, const size_t height,
		const unsigned int number_of_rays) const {

		const real invN = 1.0 / number_of_rays;

		char* texture_pixels;
		int texture_pitch;

		SDL_LockTexture(texture, NULL, (void**) &texture_pixels, &texture_pitch);
		const unsigned int padding = texture_pitch % 3;
        
		unsigned int index = 0;

		for (size_t j = 0; j < height; j++) {
            for (size_t i = 0; i < width; i++) {
				const color& pixel_col = matrix[i][j];

				// const Uint8 r = std::min((Uint8) (pixel_col.get_red()   * invN), (Uint8) 255);
				// const Uint8 g = std::min((Uint8) (pixel_col.get_green() * invN), (Uint8) 255);
				// const Uint8 b = std::min((Uint8) (pixel_col.get_blue()  * invN), (Uint8) 255);
				const Uint8 r = std::min(pixel_col.get_red()   * invN, (real) 255.0f);
				const Uint8 g = std::min(pixel_col.get_green() * invN, (real) 255.0f);
				const Uint8 b = std::min(pixel_col.get_blue()  * invN, (real) 255.0f);

				texture_pixels[index] = r;
				index++;
				texture_pixels[index] = g;
				index++;
				texture_pixels[index] = b;
				index++;
            }
			index += padding;
        }
		
        SDL_UnlockTexture(texture);
	}
	
	/**
	 * Copies the rt::color matrix onto the screen, by averaging the number_of_rays colors per pixel
	 * and applying a square root to each component to increase the brightness
	*/
	void screen::copy_gamma_corrected(std::vector<std::vector<rt::color>>& matrix,
		const size_t width, const size_t height,
		const unsigned int number_of_rays) const {
			
		for (size_t i = 0; i < width; i++) {
			for (size_t j = 0; j < height; j++) {
				const rt::color& pixel_col = matrix[i][j];
				// Maxed values
				const real r = std::min(pixel_col.get_red()   / number_of_rays, (real) 255.0f);
				const real g = std::min(pixel_col.get_green() / number_of_rays, (real) 255.0f);
				const real b = std::min(pixel_col.get_blue()  / number_of_rays, (real) 255.0f);
				// Gamma corrected values
				const real gamma_corrected_r = 255.0f * sqrt(r / 255.0f);
            	const real gamma_corrected_g = 255.0f * sqrt(g / 255.0f);
            	const real gamma_corrected_b = 255.0f * sqrt(b / 255.0f);
				set_pixel(i, j, rt::color(gamma_corrected_r, gamma_corrected_g, gamma_corrected_b));
			}
		}
	}
}
