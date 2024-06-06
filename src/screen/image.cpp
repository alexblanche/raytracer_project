#include "screen/image.hpp"

#include <iostream>

namespace rt {

	/**
	 * Default constructor is protected and can be used
	 * only by inheriting classes. It is forbidden to
	 * build an image without at least its dimensions.
	 */
	image::image() {}


	/**
	 * Main constructor. Builds an image from its dimensions.
	 */
	image::image(int width, int height)	{
		//SDL_CreateWindowAndRenderer(width, height, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE, &window, &renderer);
		window = SDL_CreateWindow("Raytracer_project", 10, 10, width, height, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
		renderer = SDL_CreateRenderer(window, (-1), SDL_RENDERER_ACCELERATED);
		SDL_RenderSetVSync(renderer, 0); // 1 = Enable VSync, 0 = Disable
		SDL_RenderSetLogicalSize(renderer, width, height);
	}

	/**
	 * Copy constructor.
	 * Warning: this does not copy the actual data, so
	 * any modification to the copy will impact the original.
	 * To build a hard copy, use the copy member function.
	
	image::image(const image& img) {
		data = img.data;
		data->refcount++;
	}
	*/

	/**
	 * Destructor.
	 */
	image::~image() {
		if (renderer != NULL){
			SDL_DestroyRenderer(renderer);
		}
		if(window != NULL){
			SDL_DestroyWindow(window);
		}
		SDL_Quit();
	}

	/**
	 * Sets a pixel to a given color.
	 */
	void image::set_pixel(int x, int y, const color& c) const {
		// if(x < 0 || y < 0 || x >= width() || y >= height()) {
		// 	return;
		// }
		Uint8 r = c.get_red();
		Uint8 g = c.get_green();
		Uint8 b = c.get_blue();
		SDL_SetRenderDrawColor(renderer, r, g, b, 255);
		SDL_RenderDrawPoint(renderer, x, y);
	}

	/**
	 * Draws a line from (x1,y1) to (x2,y2) of a given color.
	 */
	void image::draw_line(int x1, int y1, int x2, int y2, const color& c) {
		Uint8 r = c.get_red();
		Uint8 g = c.get_green();
		Uint8 b = c.get_blue();
		SDL_SetRenderDrawColor(renderer, r, g, b, 255);
		SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
	}

	/**
	 * Draws a rectangle (uses draw_line) of a given color.
	 */
	void image::draw_rect(int x1, int y1, int x2, int y2, const color& c) {
		Uint8 r = c.get_red();
		Uint8 g = c.get_green();
		Uint8 b = c.get_blue();
		SDL_SetRenderDrawColor(renderer, r, g, b, 255);

		SDL_Rect rct = SDL_Rect {x1, y1, x2-x1+1, y2-y1+1};
		SDL_RenderDrawRect(renderer, &rct);
	}

	/**
	 * Draws a filled rectangle of a given color.
	 */
	void image::fill_rect(int x1, int y1, int x2, int y2, const color& c) {

		Uint8 r = c.get_red();
		Uint8 g = c.get_green();
		Uint8 b = c.get_blue();
		SDL_SetRenderDrawColor(renderer, r, g, b, 255);

		SDL_Rect rct = SDL_Rect {x1, y1, x2-x1+1, y2-y1+1};
		SDL_RenderFillRect(renderer, &rct);
	}

	/**
	 * Erases the content of the buffer
	 */
	void image::clear() const {
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
	}
}
