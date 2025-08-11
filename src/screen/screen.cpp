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
				// case SDL_MOUSEBUTTONDOWN:
				// 	printf("\nX = %d, Y = %d", event.button.x, event.button.y);
				// 	break;
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
			const std::vector<color>& line = matrix[i];
            for (size_t j = 0; j < height; j++) {
				const color& pixel_col = line[j];
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
		/*
		unsigned int index = 0;

		for (size_t j = 0; j < height; j++) {
            for (size_t i = 0; i < width; i++) {
				const color& pixel_col = matrix[i][j];
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
		*/
		
		const unsigned int shift = 3 * width + padding;
		for (size_t i = 0; i < width; i++) {
			const std::vector<color>& line = matrix[i];
			const unsigned int threei = 3 * i;
            for (size_t j = 0; j < height; j++) {
				const color& pixel_col = line[j];
				const Uint8 r = std::min(pixel_col.get_red()   * invN, (real) 255.0f);
				const Uint8 g = std::min(pixel_col.get_green() * invN, (real) 255.0f);
				const Uint8 b = std::min(pixel_col.get_blue()  * invN, (real) 255.0f);

				unsigned int index = j * shift + threei;
				texture_pixels[index] = r;
				index++;
				texture_pixels[index] = g;
				index++;
				texture_pixels[index] = b;
				index++;
            }
        }
		
        SDL_UnlockTexture(texture);
	}

	/* Copy matrix to the screen with gamma correction */
	void screen::fast_copy_gamma(std::vector<std::vector<color>>& matrix,
		const size_t width, const size_t height,
		const unsigned int number_of_rays, const real gamma) const {

		const real invN = 1.0 / number_of_rays;
		const real inv255 = 1.0 / 255.0;

		char* texture_pixels;
		int texture_pitch;

		SDL_LockTexture(texture, NULL, (void**) &texture_pixels, &texture_pitch);
		const unsigned int padding = texture_pitch % 3;
		/*
		unsigned int index = 0;

		for (size_t j = 0; j < height; j++) {
            for (size_t i = 0; i < width; i++) {
				const color& pixel_col = matrix[i][j];
				const real red_val   = pixel_col.get_red()   * invN;
				const real green_val = pixel_col.get_green() * invN;
				const real blue_val  = pixel_col.get_blue()  * invN;
				const real gamma_corrected_red   = pow((red_val   * inv255), gamma) * 255.0f;
				const real gamma_corrected_green = pow((green_val * inv255), gamma) * 255.0f;
				const real gamma_corrected_blue  = pow((blue_val  * inv255), gamma) * 255.0f;
				const Uint8 r = std::min(gamma_corrected_red,   (real) 255.0f);
				const Uint8 g = std::min(gamma_corrected_green, (real) 255.0f);
				const Uint8 b = std::min(gamma_corrected_blue,  (real) 255.0f);

				texture_pixels[index] = r;
				index++;
				texture_pixels[index] = g;
				index++;
				texture_pixels[index] = b;
				index++;
            }
			index += padding;
        }
		*/
		
		const unsigned int shift = 3 * width + padding;
		for (size_t i = 0; i < width; i++) {
			const std::vector<color>& line = matrix[i];
			const unsigned int threei = 3 * i;
            for (size_t j = 0; j < height; j++) {
				const color& pixel_col = line[j];
				const real red_val   = pixel_col.get_red()   * invN;
				const real green_val = pixel_col.get_green() * invN;
				const real blue_val  = pixel_col.get_blue()  * invN;
				const real gamma_corrected_red   = pow((red_val   * inv255), gamma) * 255.0f;
				const real gamma_corrected_green = pow((green_val * inv255), gamma) * 255.0f;
				const real gamma_corrected_blue  = pow((blue_val  * inv255), gamma) * 255.0f;
				const Uint8 r = std::min(gamma_corrected_red,   (real) 255.0f);
				const Uint8 g = std::min(gamma_corrected_green, (real) 255.0f);
				const Uint8 b = std::min(gamma_corrected_blue,  (real) 255.0f);

				unsigned int index = j * shift + threei;
				texture_pixels[index] = r;
				index++;
				texture_pixels[index] = g;
				index++;
				texture_pixels[index] = b;
				index++;
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
				const real cr = lr * lcorr * invN;
				const real cg = lg * lcorr * invN;
				const real cb = lb * lcorr * invN;
				const real gr = pow(cr * inv255, gamma) * 255.0f;
				const real gg = pow(cg * inv255, gamma) * 255.0f;
				const real gb = pow(cb * inv255, gamma) * 255.0f;
				
				const Uint8 r = std::min(gr, (real) 255.0f);
				const Uint8 g = std::min(gg, (real) 255.0f);
				const Uint8 b = std::min(gb, (real) 255.0f);

				unsigned int index = j * shift + threei;
				texture_pixels[index] = r;
				index++;
				texture_pixels[index] = g;
				index++;
				texture_pixels[index] = b;
				index++;
            }
        }
		
        SDL_UnlockTexture(texture);
	}
}
