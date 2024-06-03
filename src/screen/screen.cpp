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
		};
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

	/****************************************************************************************************/

	/**
	 * Copies the rt::color matrix onto the screen, by averaging the number_of_rays colors per pixel
	*/
	void screen::copy(std::vector<std::vector<rt::color>>& matrix,
		const unsigned int width, const unsigned int height,
		const unsigned int number_of_rays) const {
			
		for (unsigned int i = 0; i < width; i++) {
			for (unsigned int j = 0; j < height; j++) {
				const rt::color& pixel_col = matrix[i][j];
				// Maxed values
				const double r = std::min(pixel_col.get_red()   / number_of_rays, 255.0);
				const double g = std::min(pixel_col.get_green() / number_of_rays, 255.0);
				const double b = std::min(pixel_col.get_blue()  / number_of_rays, 255.0);
				set_pixel(i, j, rt::color(r, g, b));
			}
		}
	}
	
	/**
	 * Copies the rt::color matrix onto the screen, by averaging the number_of_rays colors per pixel
	 * and applying a square root to each component to increase the brightness
	*/
	void screen::copy_gamma_corrected(std::vector<std::vector<rt::color>>& matrix,
		const unsigned int width, const unsigned int height,
		const unsigned int number_of_rays) const {
			
		for (unsigned int i = 0; i < width; i++) {
			for (unsigned int j = 0; j < height; j++) {
				const rt::color& pixel_col = matrix[i][j];
				// Maxed values
				const double r = std::min(pixel_col.get_red()   / number_of_rays, 255.0);
				const double g = std::min(pixel_col.get_green() / number_of_rays, 255.0);
				const double b = std::min(pixel_col.get_blue()  / number_of_rays, 255.0);
				// Gamma corrected values
				const double gamma_corrected_r = 255 * sqrt(r / 255.0);
            	const double gamma_corrected_g = 255 * sqrt(g / 255.0);
            	const double gamma_corrected_b = 255 * sqrt(b / 255.0);
				set_pixel(i, j, rt::color(gamma_corrected_r, gamma_corrected_g, gamma_corrected_b));
			}
		}
	}
}
