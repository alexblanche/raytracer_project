#include "headers/rect.hpp"
#include "headers/image.hpp"

#include <iostream>

namespace rt {

	/**
	 * Default constructor is protected and can be used
	 * only by inheriting classes. It is forbidden to
	 * build an image without at least its dimensions.
	 */
	image::image() {
	}


	/**
	 * Main constructor. Builds an image from its dimensions.
	 */
	image::image(int width, int height)	{
		SDL_CreateWindowAndRenderer(width, height, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE, &window, &renderer);
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
	 * Returns a hard copy of the image.
	
	image image::copy() const {
		image cpy(width(),height());
		blit(cpy,0,0);
		return cpy;
	}
	 */

	/**
	 * Returns the color of a pixel.
	
	color image::get_pixel(int x, int y) const {
		if(x < 0 || y < 0 || x >= width() || y >= height()){
			return color(0,0,0,0);
		}
		
		Obsolete
		char* pixel = ((char*)data->pixels) + y*data->pitch + x*4;
		Uint8 r,g,b,a;
		SDL_GetRGBA(*((Uint32*)pixel), data->format, &r, &g, &b, &a);
		return color(r,g,b,a)
		
		SDL_RenderReadPixels(...);
	}
	*/

	/**
	 * Sets a pixel to a given color.
	 */
	void image::set_pixel(int x, int y, const color& c) const {
		if(x < 0 || y < 0 || x >= width() || y >= height()) {
			return;
		}
		Uint8 r = c.get_red();
		Uint8 g = c.get_green();
		Uint8 b = c.get_blue();
		SDL_SetRenderDrawColor(renderer, r, g, b, 255);
		SDL_RenderDrawPoint(renderer, x, y);
	}

	/**
	 * Copy/Paste the image's source rectangle to a given destination
	 * at given coordinates.
	
	void image::blit(image& dst, const rect& srcrect, int dstx, int dsty) const	{
		SDL_Rect dstrect;
		dstrect.x = dstx;
		dstrect.y = dsty;
		SDL_BlitSurface(data, (SDL_Rect*)(&srcrect), dst.data, &dstrect);
	}
	 */

	/**
	 * Copy/Paste time entire image to a given destination
	 * at given coordinates.
	
	void image::blit(image& dst, int dstx, int dsty) const {
		rect srcrect;
		srcrect.x = 0;
		srcrect.y = 0;
		srcrect.w = dst.data->w;
		srcrect.h = dst.data->h;
		blit(dst,srcrect,dstx,dsty);
	}
	 */

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

		rect rct = rect(x1, y1, x2-x1+1, y2-y1+1);
		SDL_RenderDrawRect(renderer, &rct);
	}

	/**
	 * Draws a filled rectangle of a given color.
	 */
	void image::fill_rect(int x1, int y1, int x2, int y2, const color& c) {
		/*
		SDL_Rect r = {(Sint16)x1, (Sint16)y1, (Uint16)(x2-x1+1), (Uint16)(y2-y1+1)};
		SDL_FillRect(data, &r, SDL_MapRGBA(data->format, c.get_red(), c.get_green(), c.get_blue(), c.get_alpha()));
		*/

		Uint8 r = c.get_red();
		Uint8 g = c.get_green();
		Uint8 b = c.get_blue();
		SDL_SetRenderDrawColor(renderer, r, g, b, 255);

		rect rct = rect(x1, y1, x2-x1+1, y2-y1+1);
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
